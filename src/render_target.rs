use std::collections::HashMap;
use rand::Rng;

/// A unique identifier that can be used to look up a `RenderTarget` in a `RenderTargetList`.
/// We use 128 bit IDs and assume that, as long as clients generate them randomly,
/// they will be unique and never collide, even across different application instances. 
#[derive(Eq, Hash, PartialEq, Debug, Clone, Copy)]
pub struct RenderTargetId(u128);

impl RenderTargetId {
    /// Generate a new random RenderTargetId
    pub fn gen() -> RenderTargetId {
        RenderTargetId(rand::thread_rng().gen())
    }
}

/// RenderTargets describe different instances
/// of the render pipeline for a given Graph.
/// You may want different render target for different render requirements,
/// for instance, a different size / shape output,
/// or a different frame rate.
/// For example,
/// you might use one render target at a low resolution for rendering previews,
/// a second render target at a resolution matching your monitor for video output,
/// and a third render target at an intermediate resolution
/// for output to a LED lighting setup.
/// 
/// RenderTargets are immutable once created; you can't change the size.
/// 
/// RenderTargets are lightweight objects and don't have associated state.
/// All per-target state is stored in other parts of the system,
/// typically indexed by RenderTargetId.
pub struct RenderTarget {
    width: u32,
    height: u32,
    dt: f32,
}

impl RenderTarget {
    /// Create a new `RenderTarget` with the given dimensions, in pixels.
    pub fn new(width: u32, height: u32, dt: f32) -> Self {
        Self {
            width,
            height,
            dt,
        }
    }

    /// Get the width of the render target, in pixels
    pub fn width(&self) -> u32 {
        self.width
    }

    /// Get the height of the render target, in pixels
    pub fn height(&self) -> u32 {
        self.height
    }

    /// Get the timestep of the render target, in seconds
    pub fn dt(&self) -> f32 {
        self.dt
    }
}

/// A `RenderTargetList` represents a set of render targets and their IDs.
/// This `RenderTargetList` object is only a description:
/// It does not contain any render state or graphics resources.
/// One use case of a RenderTargetList is passing it to `Context.paint` during rendering.
pub struct RenderTargetList {
    render_targets: HashMap<RenderTargetId, RenderTarget>,
}

impl RenderTargetList {
    /// Create an empty RenderTargetList
    pub fn new() -> Self {
        Self {
            render_targets: HashMap::new(),
        }
    }

    /// Retrieve a render target from the RenderTargetList by ID
    pub fn get(&self, id: RenderTargetId) -> Option<&RenderTarget> {
        self.render_targets.get(&id)
    }

    /// Add a render target to the list.
    /// The given ID must be unique within the list.
    pub fn insert(&mut self, id: RenderTargetId, render_target: RenderTarget) {
        assert!(!self.render_targets.contains_key(&id), "Given render_target ID already exists in render_target list");
        self.render_targets.insert(id, render_target);
    }

    /// Retrieve the RenderTargetList as a HashMap of id -> `RenderTarget`
    pub fn render_targets(&self) -> &HashMap<RenderTargetId, RenderTarget> {
        &self.render_targets
    }
}
