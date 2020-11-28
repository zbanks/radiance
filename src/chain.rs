pub struct Chain {
    size: (usize, usize),
}

impl Chain {
    /// Construct a new chain for a given texture size
    pub fn new(size: (usize, usize)) -> Result<Chain, ()> {
        Ok(Chain {
            size: size,
        })
    }
}
