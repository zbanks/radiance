use rand::Rng;
use serde::{Serialize, Deserialize};
use serde::de::Error;
use std::fmt;

/// A unique identifier that can be used to look up a `RenderTarget` in a `RenderTargetList`.
/// We use 128 bit IDs and assume that, as long as clients generate them randomly,
/// they will be unique and never collide, even across different application instances. 
#[derive(Eq, Hash, Clone, Copy, PartialEq, PartialOrd, Ord)]
pub struct RenderTargetId(u128);

impl RenderTargetId {
    /// Generate a new random RenderTargetId
    pub fn gen() -> RenderTargetId {
        RenderTargetId(rand::thread_rng().gen())
    }
}

impl fmt::Display for RenderTargetId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "rt_{}", &base64::encode(self.0.to_be_bytes())[0..22])
    }
}

impl fmt::Debug for RenderTargetId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self)
    }
}

impl Serialize for RenderTargetId {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.serialize_str(&self.to_string())
    }
}

impl<'de> Deserialize<'de> for RenderTargetId {
    fn deserialize<D>(deserializer: D) -> Result<RenderTargetId, D::Error>
        where D: serde::Deserializer<'de>
    {
        let s = String::deserialize(deserializer)?;
        if let Some(suffix) = s.strip_prefix("rt_") {
            let decoded_bytes: Vec<u8> = base64::decode(suffix).map_err(D::Error::custom)?;
            let decoded_value = <u128>::from_be_bytes(decoded_bytes.try_into().map_err(|_| D::Error::custom("render target id is wrong length"))?);
            Ok(RenderTargetId(decoded_value))
        } else {
            Err(D::Error::custom("not a valid render target id (must start with rt_)"))
        }
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
#[derive(Debug, Clone, Serialize, Deserialize)]
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
