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
/// The graph topology must be acyclic. Calling fix() will enforce this.
#[derive(Debug, Clone, Default, PartialEq, Eq, Serialize, Deserialize)]
pub struct Graph {
    pub nodes: Vec<NodeId>,
    pub edges: Vec<Edge>,
}

/// A insertion point describes a place in a graph where a subgraph can be inserted.
/// A insertion point may be:
/// * Closed: A insertion point with both an input and an output.
///   This happens when new nodes are being inserted between existing connected nodes.
///   Inserting nodes here involves removing existing edges.
///
///   Note that when an output connects to multiple inputs,
///   there are two options for constructing an insertion point:
///                 --o--> Node B
///                /
///   Node A --o--+------> Node C
///                \
///                 -----> Node D
///   Both 'o' marks are a valid insertion point between nodes A and B.
///   One is on the output of A, and insertion will affect all downstream connections.
///   This InsertionPoint will have three to_input entries.
///   The other is on the input of B, and will only affect B's branch.
///   This InsertionPoint will have one to_input entry.
/// * Half-open on the output: An insertion point that has no output.
///   This happens when nodes are inserted at output of an existing "start node"
///   (all the way towards the root.)
///   After insertion, one of the inserted nodes will be the new "start node".
/// * Half-open on the input: An insertion point that has no inputs.
///   This happens when nodes are inserted at the input of an existing leaf node.
///   After insertion, the one of the inserted nodes will be the new leaf.
/// * Open: An insertion point that has no input or output.
///   This happens when nodes are inserted as a disconnected subgraph.
///   No connections to the existing graph need to be made or broken.
#[derive(Debug, Clone, Default)]
pub struct InsertionPoint {
    pub from_output: Option<NodeId>,
    pub to_inputs: Vec<(NodeId, u32)>,
}

/// Given a list of nodes and edges,
/// removes edges that reference nonexistant nodes
/// and edges that connect to an input that already has an edge going to it.
fn remove_invalid_edges(nodes: &[NodeId], edges: &mut Vec<Edge>) {
    let mut inputs_seen = HashSet::<(NodeId, u32)>::new();
    let nodes_set: HashSet<NodeId> = nodes.iter().cloned().collect();

    edges.retain(|edge| {
        let valid = nodes_set.contains(&edge.to) &&
                    nodes_set.contains(&edge.from) &&
                    !inputs_seen.contains(&(edge.to, edge.input));

        inputs_seen.insert((edge.to, edge.input));
        valid
    });
}

/// Convert from a list of nodes and edges
/// into a mapping from each node to its input nodes.
/// Return this mapping.
fn map_inputs(nodes: &[NodeId], edges: &[Edge]) -> HashMap<NodeId, Vec<Option<NodeId>>> {
    let mut input_mapping = HashMap::<NodeId, Vec<Option<NodeId>>>::new();

    // Add every node to the map
    for node in nodes.iter() {
        input_mapping.insert(*node, Default::default());
    }

    // Add every edge to the map
    for edge in edges.iter() {
        if !input_mapping.contains_key(&edge.to) ||
           !input_mapping.contains_key(&edge.from)
        {
            continue;
        }

        let input_vec = input_mapping.get_mut(&edge.to).unwrap();

        // Ensure this input doesn't already have a connection
        if input_vec.get(edge.input as usize).cloned().flatten().is_some() {
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

    input_mapping
}

/// Convert from a list of nodes and edges
/// into a mapping from each node to its output nodes.
/// Return this mapping.
fn map_outputs(nodes: &[NodeId], edges: &[Edge]) -> HashMap<NodeId, HashSet<(NodeId, u32)>> {
    let mut output_mapping = HashMap::<NodeId, HashSet<(NodeId, u32)>>::new();

    // Add every node to the map
    for node in nodes.iter() {
        output_mapping.insert(*node, Default::default());
    }

    // Add every edge to the map
    for edge in edges.iter() {
        if !output_mapping.contains_key(&edge.to) ||
           !output_mapping.contains_key(&edge.from)
        {
            continue;
        }

        let output_set = output_mapping.get_mut(&edge.from).unwrap();
        output_set.insert((edge.to, edge.input));
    }

    output_mapping
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
    // We frequently reverse the order things are pushed to the stack.
    // This prioritizes cycle breaking towards retaining edges
    // on start nodes that appear earlier in the nodes list,
    // and on nodes connected to lower-numbered inputs.
    let mut stack: Vec<NodeId> = nodes.iter().rev().cloned().collect();
    let mut color = HashMap::<NodeId, Color>::from_iter(input_mapping.keys().map(|x| (*x, Color::White)));

    while let Some(&n) = stack.last() {
        let cn = color.get(&n).unwrap();
        match cn {
            Color::White => {
                color.insert(n, Color::Grey);
                for (i, opt_m) in input_mapping.get(&n).unwrap().iter().rev().enumerate() {
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

/// Walk the graph from a starting node until a disconnected input 0 is found.
/// Return the node with disconnected input 0.
fn first_input(input_mapping: &HashMap<NodeId, Vec<Option<NodeId>>>, start_node: NodeId) -> NodeId {
    let mut node = start_node;
    loop {
        match input_mapping.get(&node).and_then(|input_vec| input_vec.get(0).cloned().flatten()) {
            Some(n) => {node = n;}
            None => {break;}
        }
    }
    node
}

/// Use DFS to compute a topological ordering of the nodes in a graph (represented as a mapping.)
/// The input mapping must be acyclic or this function will panic.
/// This sort is stable (calling topo_sort_nodes a second time should be idempotent.)
pub fn topo_sort_nodes(start_nodes: &[NodeId], input_mapping: &HashMap<NodeId, Vec<Option<NodeId>>>) -> Vec<NodeId> {
    // Topo-sort using DFS.
    // Use a "white-grey-black" node coloring approach.
    // https://stackoverflow.com/a/62971341

    enum Color {
        White, // Node is not visited
        Grey, // Node is on the path that is being explored
        Black, // Node is visited
    }

    // Push start nodes onto the stack for DFS
    // This prioritizes exploring start nodes that appear earlier in the nodes list,
    // as well as nodes connected to lower-numbered inputs.
    let mut stack: Vec<NodeId> = start_nodes.iter().rev().cloned().collect();
    let mut color = HashMap::<NodeId, Color>::from_iter(input_mapping.keys().map(|x| (*x, Color::White)));
    let mut topo_order = Vec::<NodeId>::new();

    while let Some(&n) = stack.last() {
        let cn = color.get(&n).unwrap();
        match cn {
            Color::White => {
                color.insert(n, Color::Grey);
                for m in input_mapping.get(&n).unwrap().iter().rev().flatten() {
                    let cm = color.get(m).unwrap();
                    match cm {
                        Color::White => {
                            stack.push(*m);
                        },
                        Color::Grey => {
                            // This edge creates a cycle! Panic!
                            panic!("Cycle detected in graph");
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
                topo_order.push(n);
            },
            Color::Black => {
                panic!("DFS integrity error"); // Black nodes should never be pushed to the stack
            },
        }
    }
    topo_order
}

impl Graph {
    /// Compute the start nodes and input mapping of a well-formed DAG.
    /// This function may return unexpected results
    /// if the input is not a well-formed DAG.
    /// The response will be a pair: (start_nodes, input_mapping)
    pub fn mapping(&self) -> (Vec<NodeId>, HashMap<NodeId, Vec<Option<NodeId>>>) {
        let input_mapping = map_inputs(&self.nodes, &self.edges);
        let start_nodes = start_nodes(&self.nodes, &input_mapping);

        (start_nodes, input_mapping)
    }

    /// Sort nodes and repair edges in a graph.
    /// If the graph contains edges referencing nonexistant nodes, they will be removed.
    /// If a node input has multiple incoming edges, all but one will be removed.
    /// If the graph is cyclic, repair it by removing edges until it is not cyclic.
    /// The node list will be put in topological order.
    pub fn fix(&mut self) {
        let orig_n_edges = self.edges.len();
        remove_invalid_edges(&self.nodes, &mut self.edges);
        let edges_removed = self.edges.len() - orig_n_edges;
        if edges_removed > 0 {
            println!("Removed {} invalid edges", edges_removed);
        }

        let input_mapping = map_inputs(&self.nodes, &self.edges);

        let cyclic_edges = find_cycles(&self.nodes, &input_mapping);

        if !cyclic_edges.is_empty() {
            // Remove edges that cause cycles
            self.edges.retain(|e| !cyclic_edges.contains(e));

            println!("Removed {} edges to break cycles", cyclic_edges.len());
        }

        let start_nodes = start_nodes(&self.nodes, &input_mapping);
        self.nodes = topo_sort_nodes(&start_nodes, &input_mapping);
    }

    /// Delete a set of nodes from the graph.
    pub fn delete_nodes(&mut self, nodes: &HashSet<NodeId>) {
        let input_mapping = map_inputs(&self.nodes, &self.edges);
        let output_mapping = map_outputs(&self.nodes, &self.edges);

        let subgraph_nodes: Vec<NodeId> = self.nodes.iter().filter(|n| nodes.contains(n)).cloned().collect();
        let subgraph_input_mapping = map_inputs(&subgraph_nodes, &self.edges);
        let start_nodes = start_nodes(&subgraph_nodes, &subgraph_input_mapping);

        for &start_node in start_nodes.iter() {
            // If we walk the graph, starting from each start node,
            // we find one connected component of the subgraph to delete
            let end_node = first_input(&subgraph_input_mapping, start_node);
            let outgoing_connections = output_mapping.get(&start_node).unwrap();
            let incoming_connection = input_mapping.get(&end_node).unwrap().get(0).cloned().flatten();

            if let Some(from) = incoming_connection {
                for &(to, input) in outgoing_connections.iter() {
                    // Push a "bypass" edge to hop over the nodes being deleted
                    self.edges.push(Edge {
                        from,
                        to,
                        input,
                    });
                }
            }
        }

        self.nodes.retain(|n| !nodes.contains(n));
        self.edges.retain(|e| !nodes.contains(&e.from) && !nodes.contains(&e.to));
    }

    /// Add a node to the graph
    /// and make appropriate connections
    /// so that it appears at the given insertion point.
    pub fn insert_node(&mut self, node: NodeId, insertion_point: &InsertionPoint) {
        self.nodes.push(node);
        if let Some(from_output) = insertion_point.from_output {
            // If we have both an output and inputs,
            // remove any existing edges in the graph
            self.edges.retain(|e| 
                e.from != from_output &&
                !insertion_point.to_inputs.contains(&(e.to, e.input))
            );

            // Wire up the incoming edge
            self.edges.push(Edge {
                from: from_output,
                to: node,
                input: 0,
            });
        }

        // Wire up the outgoing edges
        for &(to, input) in insertion_point.to_inputs.iter() {
            self.edges.push(Edge {
                from: node,
                to,
                input,
            });
        }
    }
}
