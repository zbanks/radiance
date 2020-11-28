use crate::chain::Chain;

struct Context {
    chains: Vec<Chain>,
}

impl Context {
    fn new() -> Context {
        Context {
            chains: Vec::new(),
        }
    }
}
