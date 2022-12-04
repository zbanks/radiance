use rand::Rng;
use std::collections::{HashMap, HashSet};
use serde::{Serialize, Deserialize};
use serde::de::Error;
use std::fmt;

/// A unique identifier that can be used to look up a `Node`.
/// We use 128 bit IDs and assume that, as long as clients generate them randomly,
/// they will be unique and never collide, even across different application instances. 
#[derive(Eq, Hash, Clone, Copy, PartialEq, PartialOrd, Ord)]
pub struct NodeId(u128);

impl NodeId {
    /// Generate a new random NodeId
    pub fn gen() -> NodeId {
        NodeId(rand::thread_rng().gen())
    }
}

impl fmt::Display for NodeId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "node_{}", &base64::encode(self.0.to_be_bytes())[0..22])
    }
}

impl fmt::Debug for NodeId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.to_string())
    }
}

impl Serialize for NodeId {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.serialize_str(&self.to_string())
    }
}

impl<'de> Deserialize<'de> for NodeId {
    fn deserialize<D>(deserializer: D) -> Result<NodeId, D::Error>
        where D: serde::Deserializer<'de>
    {
        let s = String::deserialize(deserializer)?;
        if s.starts_with("node_") {
            let decoded_bytes: Vec<u8> = base64::decode(&s[5..]).map_err(D::Error::custom)?;
            let decoded_value = <u128>::from_be_bytes(decoded_bytes.try_into().map_err(|_| D::Error::custom("node id is wrong length"))?);
            Ok(NodeId(decoded_value))
        } else {
            Err(D::Error::custom("not a valid node id (must start with node_)"))
        }
    }
}

/// An edge in the graph is identified by a source `NodeId` ("from"),
/// a sink `NodeId` ("to"), and which input of the sink node to use ("input").
#[derive(Debug, Clone, Serialize, Deserialize, Eq, Hash, PartialEq)]
pub struct Edge {
    pub from: NodeId,
    pub to: NodeId,
    pub input: u32,
}

/// A Graph stores node connectivity information.
/// Each node is identified by a `NodeId` and is stored in a sorted list.
/// The ordering of the list does not affect updating and painting,
/// but may be used for when visualizing the graph in the UI.
/// 
/// The graph topology must be acyclic. Calling topology() will check this.
#[derive(Debug, Clone, Default, PartialEq, Eq, Serialize, Deserialize)]
pub struct Graph {
    pub nodes: Vec<NodeId>,
    pub edges: Vec<Edge>,
}

/// Convert from a list of nodes and edges
/// into a mapping from node to ints input nodes.
/// Return this mapping, along with a set of edges that were not valid.
fn map_inputs(nodes: &[NodeId], edges: &[Edge]) -> (HashMap<NodeId, Vec<Option<NodeId>>>, HashSet<Edge>) {
    let mut input_mapping = HashMap::<NodeId, Vec<Option<NodeId>>>::new();
    let mut invalid_edges = HashSet::<Edge>::new();

    // Add every node to the map
    for node in nodes.iter() {
        input_mapping.insert(*node, Default::default());
    }

    // Add every edge to the map
    for edge in edges.iter() {
        if !input_mapping.contains_key(&edge.to) ||
           !input_mapping.contains_key(&edge.from)
        {
            invalid_edges.insert(edge.clone());
            continue;
        }

        let input_vec = input_mapping.get_mut(&edge.to).unwrap();

        // Ensure this input doesn't already have a connection
        if input_vec.get(edge.input as usize).cloned().flatten().is_some() {
            invalid_edges.insert(edge.clone());
            continue;
        }

        // Ensure vec has enough space to add our entry
        if input_vec.len() <= edge.input as usize {
            for _ in 0..=edge.input as usize - input_vec.len() {
                input_vec.push(None);
            }
        }

        // Add to vec
        input_vec[edge.input as usize] = Some(edge.from);
    }

    (input_mapping, invalid_edges)
}

/// Find cycles in the graph.
/// Return which edges need to be removed from the graph in order to break all cycles.
fn find_cycles(nodes: &[NodeId], input_mapping: &HashMap<NodeId, Vec<Option<NodeId>>>) -> HashSet<Edge> {
    let mut cyclic_edges = HashSet::<Edge>::new();

    // Identify cycles using DFS.
    // Use a "white-grey-black" node coloring approach
    // to detect cycles
    // https://stackoverflow.com/a/62971341

    enum Color {
        White, // Node is not visited
        Grey, // Node is on the path that is being explored
        Black, // Node is visited
    }

    // Push all nodes onto the stack for DFS
    let mut stack: Vec<NodeId> = nodes.to_vec();
    let mut color = HashMap::<NodeId, Color>::from_iter(input_mapping.keys().map(|x| (*x, Color::White)));

    while let Some(&n) = stack.last() {
        let cn = color.get(&n).unwrap();
        match cn {
            Color::White => {
                color.insert(n, Color::Grey);
                for (i, opt_m) in input_mapping.get(&n).unwrap().iter().enumerate() {
                    let m = match opt_m {
                        Some(m) => m,
                        None => continue,
                    };

                    let cm = color.get(m).unwrap();
                    match cm {
                        Color::White => {
                            stack.push(*m);
                        },
                        Color::Grey => {
                            // This edge creates a cycle! Add it to the list of edges to remove
                            cyclic_edges.insert(Edge {
                                from: *m,
                                to: n,
                                input: i as u32,
                            });
                        },
                        Color::Black => {
                            // Already visited; no action necessary
                        },
                    }
                }
            },
            Color::Grey => {
                color.insert(n, Color::Black);
                stack.pop();
            },
            Color::Black => {
                // Some of the original nodes that were pushed will become colored black,
                // we can ignore them as they have been explored already.
                stack.pop();
            },
        }
    }

    cyclic_edges
}

/// Computes the set of "start nodes", nodes that have no ouputs.
/// These nodes can be used as a starting point for a DFS,
/// assuming the graph is acyclic.
fn start_nodes(nodes: &[NodeId], input_mapping: &HashMap<NodeId, Vec<Option<NodeId>>>) -> Vec<NodeId> {
    let mut start_nodes: HashSet<NodeId> = input_mapping.keys().cloned().collect();
    for input_vec in input_mapping.values() {
        for maybe_input in input_vec {
            match maybe_input {
                Some(input) => {
                    start_nodes.remove(input);
                },
                None => {},
            }
        }
    }
    // Convert start_nodes to vec in a stable fashion
    // (returned start_nodes will appear in same order as the parameter 'nodes')
    let start_nodes: Vec<NodeId> = nodes.iter().filter(|x| start_nodes.contains(x)).cloned().collect();

    start_nodes
}

impl Graph {
    /// Compute the start nodes and input mapping of a well-formed DAG.
    /// This function may panic or return unexpected results
    /// if the input is not a well-formed DAG.
    /// The response will be a pair: (start_nodes, input_mapping)
    pub fn mapping(&self) -> (Vec<NodeId>, HashMap<NodeId, Vec<Option<NodeId>>>) {
        let (input_mapping, invalid_edges) = map_inputs(&self.nodes, &self.edges);
        if !invalid_edges.is_empty() {
            panic!("Graph contains edges that reference nonexistant nodes");
        }

        let start_nodes = start_nodes(&self.nodes, &input_mapping);

        (start_nodes, input_mapping)
    }

    /// Repair a graph if necessary.
    /// If the graph contains edges referencing nonexistant nodes, they will be removed.
    /// If a node input has multiple incoming edges, all but one will be removed.
    /// If the graph is cyclic, repair it by removing edges until it is not cyclic.
    /// Returns an input mapping (map from NodeIds to their inputs)
    pub fn repair(&mut self) -> HashMap<NodeId, Vec<Option<NodeId>>> {
        let (input_mapping, invalid_edges) = map_inputs(&self.nodes, &self.edges);

        if !invalid_edges.is_empty() {
            // Remove edges that reference nonexistant nodes
            self.edges.retain(|e| !invalid_edges.contains(e));
        }

        let cyclic_edges = find_cycles(&self.nodes, &input_mapping);

        if !cyclic_edges.is_empty() {
            // Remove edges that cause cycles
            self.edges.retain(|e| !cyclic_edges.contains(e));

            // Recompute input_mapping since we removed edges
            let (input_mapping, invalid_edges) = map_inputs(&self.nodes, &self.edges);
            assert!(invalid_edges.is_empty(), "Invalid edges were not removed");
            input_mapping
        } else {
            input_mapping
        }
    }

    /// Delete a set of nodes from the graph.
    /// TODO attempt to preserve connectivity.
    pub fn delete_nodes(&mut self, nodes: &HashSet<NodeId>) {
        let length = self.nodes.len();
        self.nodes.retain(|n| !nodes.contains(n));
        if self.nodes.len() != length {
            // If we actually deleted any nodes,
            // remove dangling edges
            let (_, invalid_edges) = map_inputs(&self.nodes, &self.edges);
            self.edges.retain(|e| !invalid_edges.contains(e));
        }
    }
}
