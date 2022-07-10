use std::collections::HashMap;
use rand::Rng;

#[derive(Eq, Hash, PartialEq, Debug, Clone, Copy)]
pub struct ChainId(u128);

impl ChainId {
    pub fn gen() -> ChainId {
        ChainId(rand::thread_rng().gen())
    }
}

pub struct Chain {
    width: u32,
    height: u32,
}

impl Chain {
    pub fn new(width: u32, height: u32) -> Self {
        Self {
            width,
            height,
        }
    }

    pub fn width(&self) -> u32 {
        self.width
    }

    pub fn height(&self) -> u32 {
        self.height
    }
}

pub struct Chains {
    pub chains: HashMap<ChainId, Chain>,
}

impl Chains {
    pub fn new() -> Self {
        Self {
            chains: HashMap::new(),
        }
    }

    pub fn chains(&self) -> &HashMap<ChainId, Chain> {
        &self.chains
    }

    pub fn insert_chain(&mut self, id: ChainId, chain: Chain) {
        assert!(!self.chains.contains_key(&id), "Given chain ID already exists in graph");
        self.chains.insert(id, chain);
    }
}
