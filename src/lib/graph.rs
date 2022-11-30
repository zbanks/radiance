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

/// Useful topological properties computed from a Graph
#[derive(Debug, Clone, Default)]
pub struct GraphTopology {
    /// A mapping from a node to its inputs (dependencies)
    /// The indices in the returned Vec will correspond to the numbered inputs of the node
    /// and can be used for rendering.
    pub input_mapping: HashMap<NodeId, Vec<Option<NodeId>>>,
    pub start_nodes: Vec<NodeId>,
    pub topo_order: Vec<NodeId>,
}

impl Graph {
    pub fn topology(&self) -> Result<GraphTopology, &'static str> {
        // 1. compute the input mapping

        let mut input_mapping = HashMap::<NodeId, Vec<Option<NodeId>>>::new();

        // Add every node to the map
        for node in self.nodes.iter() {
            input_mapping.insert(*node, Default::default());
        }

        // Add every edge to the map
        for edge in self.edges.iter() {
            let input_vec = input_mapping.get_mut(&edge.to).unwrap();

            // Ensure vec has enough space to add our entry
            if input_vec.len() <= edge.input as usize {
                for _ in 0..=edge.input as usize - input_vec.len() {
                    input_vec.push(None);
                }
            }

            // Add to vec
            input_vec[edge.input as usize] = Some(edge.from);
        }

        // Later parts of this function don't work properly
        // if the graph is totally empty.
        // Return successfully in this case.
        if input_mapping.len() == 0 {
            return Ok(Default::default());
        }

        // 2. compute the start nodes and topo order

        // Find the "start" nodes: the nodes with no outputs
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
        if start_nodes.len() == 0 {
            return Err("Graph is cyclic")
        }
        // Convert start_nodes to vec in a stable fashion
        // (returned start_nodes will appear in same order as the parameter 'nodes')
        let start_nodes: Vec<NodeId> = self.nodes.iter().filter(|x| start_nodes.contains(x)).cloned().collect();

        // Topo-sort using DFS
        // Use a "white-grey-black" node coloring approach
        // to detect cycles
        // https://stackoverflow.com/a/62971341

        enum Color {
            White, // Node is not visited
            Grey, // Node is on the path that is being explored
            Black, // Node is visited
        }

        // Push "start" nodes onto the stack for DFS
        let mut stack: Vec<NodeId> = start_nodes.clone();
        let mut color = HashMap::<NodeId, Color>::from_iter(input_mapping.keys().map(|x| (*x, Color::White)));
        let mut topo_order = Vec::<NodeId>::new();

        while let Some(&n) = stack.last() {
            let cn = color.get(&n).unwrap();
            match cn {
                Color::White => {
                    color.insert(n, Color::Grey);
                    for m in input_mapping.get(&n).unwrap().iter().flatten() {
                        let cm = color.get(m).unwrap();
                        match cm {
                            Color::White => {
                                stack.push(*m);
                            },
                            Color::Grey => {
                                return Err("Graph is cyclic");
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
                    panic!("DFS error; visited node on the stack");
                },
            }
        }

        if !color.values().all(|c| match c {Color::Black => true, _ => false}) {
            // Isolated nodes connected to themself will not appear in start_nodes
            // and will not be visited
            return Err("Graph is cyclic");
        }

        Ok(GraphTopology {
            input_mapping,
            start_nodes,
            topo_order,
        })
    }
}
