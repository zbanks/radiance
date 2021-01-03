use std::collections::{HashMap, HashSet};

#[derive(Debug, PartialEq, Eq, Hash)]
pub struct Edge {
    pub from: u32,
    pub to: u32,
    pub input: u32,
}

pub trait Node {
}

pub struct DAG<N: Node> {
    pub nodes: HashMap::<u32, N>,
    pub edges: HashSet::<Edge>,
}

impl<N: Node> DAG<N> {
    pub fn new() -> Self {
        DAG {
            nodes: HashMap::new(),
            edges: HashSet::new(),
        }
    }

    /// Returns a vector of the inputs for every node in the graph.
    pub fn node_inputs(&self) -> HashMap<u32, Vec<Option<u32>>> {
        let mut inputs = HashMap::new();
        for edge in &self.edges {
            let input_vec = inputs.entry(edge.to).or_insert(Vec::new());
            // Grow the vector if necessary
            for _ in input_vec.len()..=(edge.input as usize) {
                input_vec.push(None);
            }
            input_vec[edge.input as usize] = Some(edge.from);
        }
        inputs
    }

    /// Returns a vector of the graph nodes in topological order.
    pub fn topo_order(&self) -> Vec<u32> {
        vec![0, 1, 2] // XXX
    }
}
