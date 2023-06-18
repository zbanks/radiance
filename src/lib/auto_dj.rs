use crate::effect_node::EffectNodeProps;
use crate::graph::{Edge, NodeId};
use crate::placeholder_node::PlaceholderNodeProps;
use crate::props::{NodeProps, Props};

use rand::{prelude::SliceRandom, Rng};

const STABLE_EFFECT_COUNT: usize = 6;

const STABLE_TIMER_MIN: usize = 100;
const STABLE_TIMER_MAX: usize = 400;
const CROSSFADE_TIMER: usize = 400;

#[derive(Debug)]
struct AutoDJEffectDescriptor {
    name: &'static str,
    category: AutoDJEffectCategory,
    intensity_min: f32,
    intensity_max: f32,
    random_frequency: bool,
}

#[derive(Debug, PartialEq)]
enum AutoDJEffectCategory {
    DontUse,
    Generative,
    ComplectSpace,
    ComplectColor,
    SimplifySpace,
    SimplifyColor,
}

const EFFECT_DESCRIPTOR_DEFAULT: AutoDJEffectDescriptor = AutoDJEffectDescriptor {
    name: "",
    category: AutoDJEffectCategory::DontUse,
    intensity_min: 0.2,
    intensity_max: 1.,
    random_frequency: true,
};

const EFFECTS: &[AutoDJEffectDescriptor] = &[
    AutoDJEffectDescriptor {
        name: "afix",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "afixhighlight",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "allwhite",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "attractor",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "bespeckle",
        category: AutoDJEffectCategory::SimplifySpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "bespecklep",
        category: AutoDJEffectCategory::SimplifySpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "blowout",
        category: AutoDJEffectCategory::SimplifyColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "brighten",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "broken",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "bstrobe",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "bumpsphere",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "bwave",
        category: AutoDJEffectCategory::ComplectColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "cedge",
        category: AutoDJEffectCategory::SimplifySpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "cga1",
        category: AutoDJEffectCategory::DontUse, // Needs work
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "chansep",
        category: AutoDJEffectCategory::ComplectColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "circle",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "coin",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "color",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "composite",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "count",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "crack",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    }, // Doesn't blend well
    AutoDJEffectDescriptor {
        name: "crossfader",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "crt",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "csmooth",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    }, // Could use, maybe
    AutoDJEffectDescriptor {
        name: "cube",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "cyan",
        category: AutoDJEffectCategory::Generative,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "cycle2",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "cylinder",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "deblack",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "delace",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "delay",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "depolar",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "desat",
        category: AutoDJEffectCategory::SimplifyColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "diodefoh",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "diodelpf",
        category: AutoDJEffectCategory::SimplifySpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "distort",
        category: AutoDJEffectCategory::ComplectSpace,
        random_frequency: false,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "droste",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "dunkirk",
        category: AutoDJEffectCategory::SimplifyColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "dwwave",
        category: AutoDJEffectCategory::Generative,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "edge",
        category: AutoDJEffectCategory::SimplifySpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "emboss",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "eye",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "filmstop",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "fire",
        category: AutoDJEffectCategory::Generative,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "fireball",
        category: AutoDJEffectCategory::Generative,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "flippy",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "flow",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "flower",
        intensity_min: 0.4, // Looks like a penis at <0.3
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "flowing",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "fly",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "fractalzoom",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "gamma",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "gay",
        category: AutoDJEffectCategory::Generative,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "glitch",
        category: AutoDJEffectCategory::DontUse, // Too good--makes radiance look broken
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "greenaway",
        category: AutoDJEffectCategory::SimplifyColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "greenscreen",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "green",
        category: AutoDJEffectCategory::SimplifyColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "gstrobe",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "halftone",
        category: AutoDJEffectCategory::DontUse, // Exaerbates high spatial frequencies
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "heart",
        category: AutoDJEffectCategory::Generative,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "hpass",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "hpf",
        category: AutoDJEffectCategory::DontUse, // Too darkening
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "hpixelate",
        random_frequency: false, // Too bouncy
        intensity_max: 0.5,      // Boring at higher intensities
        category: AutoDJEffectCategory::ComplectSpace, // Too powerful for late in the chain
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "hue",
        category: AutoDJEffectCategory::ComplectColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "id",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "inception",
        category: AutoDJEffectCategory::DontUse, // Some weird high frequencies when ramping up
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "interstellar",
        category: AutoDJEffectCategory::Generative,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "invertl",
        category: AutoDJEffectCategory::DontUse,
        random_frequency: false,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "jet",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "kaleidoscope",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "kmeans",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "lathe",
        category: AutoDJEffectCategory::DontUse, // Exacerbates high spatial frequencies
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "life",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "litebrite",
        category: AutoDJEffectCategory::DontUse, // Exacerbates high spatial frequencies
        random_frequency: false,                 // Too bouncey
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "loopy",
        category: AutoDJEffectCategory::Generative,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "lorenz",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "lpass",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "lpf",
        category: AutoDJEffectCategory::SimplifySpace,
        random_frequency: false,
        intensity_max: 0.8, // Don't let it completely stop the image
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "ltohs",
        random_frequency: false, // Too strobey
        category: AutoDJEffectCategory::ComplectColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "maze",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "melt",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "mirror",
        category: AutoDJEffectCategory::ComplectSpace,
        random_frequency: false,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "monochrome",
        category: AutoDJEffectCategory::SimplifyColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "negative",
        category: AutoDJEffectCategory::DontUse,
        random_frequency: false,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "nibble",
        category: AutoDJEffectCategory::DontUse,
        random_frequency: false,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "nogreen",
        category: AutoDJEffectCategory::SimplifyColor,
        random_frequency: false,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "no",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "onblack",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "oscope",
        category: AutoDJEffectCategory::Generative,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "outline",
        category: AutoDJEffectCategory::ComplectColor,
        random_frequency: false,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "pan",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "pink",
        category: AutoDJEffectCategory::Generative,
        random_frequency: false,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "pixelate",
        category: AutoDJEffectCategory::DontUse,
        random_frequency: false,
        ..EFFECT_DESCRIPTOR_DEFAULT
    }, // Amplifies high frequencies
    AutoDJEffectDescriptor {
        name: "polar",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "polygon",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "posterh",
        category: AutoDJEffectCategory::SimplifyColor,
        random_frequency: false,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "posterize",
        category: AutoDJEffectCategory::SimplifyColor,
        random_frequency: false,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "projector",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    }, // Too computationally intensive
    AutoDJEffectDescriptor {
        name: "purple",
        category: AutoDJEffectCategory::Generative,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "qcircle",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    }, // Not generative enough
    AutoDJEffectDescriptor {
        name: "qlpf",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    }, // Prefer lpf
    AutoDJEffectDescriptor {
        name: "rainblur",
        category: AutoDJEffectCategory::ComplectColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "rainbow",
        category: AutoDJEffectCategory::ComplectColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "random",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "randy",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "rblurb",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "rblur",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    }, // Prefer rblurb
    AutoDJEffectDescriptor {
        name: "red",
        category: AutoDJEffectCategory::SimplifyColor,
        random_frequency: false,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "rekt",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    }, // Not generative enough
    AutoDJEffectDescriptor {
        name: "resat",
        category: AutoDJEffectCategory::ComplectColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "resistor",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "rfuzz",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "rgbmask",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "rjump",
        category: AutoDJEffectCategory::ComplectColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "rolling",
        category: AutoDJEffectCategory::DontUse, // Boring
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "rotate",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "rsheen",
        category: AutoDJEffectCategory::ComplectColor,
        intensity_min: 0.1,
        intensity_max: 0.2, // Too powerful
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "saturate",
        category: AutoDJEffectCategory::ComplectColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "scramble",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "sethue",
        category: AutoDJEffectCategory::SimplifyColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "shake",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "slide",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "smoke",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "smoky",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "smooth",
        category: AutoDJEffectCategory::DontUse, // Boring
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "snowcrash",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "solitaire",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "speckle",
        category: AutoDJEffectCategory::SimplifySpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "spin",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "squares",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "sscan",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "starfield",
        category: AutoDJEffectCategory::Generative,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "still",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "strange",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "stripes",
        category: AutoDJEffectCategory::DontUse, // Too high frequency
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "stripey",
        category: AutoDJEffectCategory::DontUse, // Breaks aesthetic a bit too much
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "strobe",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "subpixel",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "survey",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "swipe",
        category: AutoDJEffectCategory::DontUse, // Boring
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "tesselate.wgsl",
        category: AutoDJEffectCategory::DontUse,
        random_frequency: false,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "test",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "threedee",
        category: AutoDJEffectCategory::ComplectColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "tileable",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "tile",
        category: AutoDJEffectCategory::DontUse, // Tesselate is better
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "tilt",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "tunnel",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "uvmapcross",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "uvmapcycle4",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "uvmapid",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "uvmapselfn",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "uvmapself",
        category: AutoDJEffectCategory::ComplectSpace,
        random_frequency: false,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "uvmap",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "vu",
        category: AutoDJEffectCategory::DontUse, // It can be very bouncy/strobey since it can change faster than the chosen frequency
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "warble",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "wave",
        category: AutoDJEffectCategory::Generative,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "wavy",
        category: AutoDJEffectCategory::ComplectSpace,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "wobsphere",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "wwave",
        category: AutoDJEffectCategory::Generative,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "yaw",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "yellow",
        category: AutoDJEffectCategory::Generative,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "yuvblur",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "yuvchansep",
        category: AutoDJEffectCategory::ComplectColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "yuvmapd",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "yuvmapid",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "yuvmap",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "yuvposter",
        category: AutoDJEffectCategory::SimplifyColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "yuvrot",
        category: AutoDJEffectCategory::ComplectColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "yuvsat",
        category: AutoDJEffectCategory::SimplifyColor,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
    AutoDJEffectDescriptor {
        name: "zoomin",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    }, // Could use, maybe
    AutoDJEffectDescriptor {
        name: "zoomout",
        category: AutoDJEffectCategory::DontUse,
        ..EFFECT_DESCRIPTOR_DEFAULT
    },
];

// Frequency options for randomly selected frequency
const RANDOM_FREQUENCY_OPTIONS: &[Option<f32>] = &[None, Some(0.25), Some(0.5), Some(1.)];

#[derive(Debug)]
pub struct AutoDJ {
    state: AutoDJState,
}

#[derive(Debug)]
enum AutoDJState {
    Pending,
    Running(AutoDJRunning),
    Broken,
}

#[derive(Debug)]
struct AutoDJRunning {
    left_placeholder: NodeId,
    right_placeholder: NodeId,
    action: AutoDJAction,
}

#[derive(Debug)]
enum AutoDJAction {
    Stable(AutoDJStable),
    Crossfade(AutoDJCrossfade),
}

#[derive(Clone, Debug)]
struct AutoDJEffect {
    id: NodeId,
    target_intensity: f32,
}

#[derive(Debug)]
struct AutoDJStable {
    effects: [AutoDJEffect; STABLE_EFFECT_COUNT],
    timer: usize, // Countdown timer to crossfade
}

#[derive(Debug)]
struct AutoDJCrossfade {
    effects: [AutoDJEffect; STABLE_EFFECT_COUNT + 1],
    fade_in_ix: usize,
    fade_out_ix: usize,
    start_timer: usize, // What the timer started at
    timer: usize,       // Countdown timer to stable
}

impl AutoDJ {
    pub fn new() -> Self {
        AutoDJ {
            state: AutoDJState::Pending,
        }
    }

    fn pick_new_node(ix: usize) -> (NodeProps, AutoDJEffect) {
        let mut rng = rand::thread_rng();

        let descriptor_options = match ix {
            0 => EFFECTS
                .iter()
                .filter(|e| e.category == AutoDJEffectCategory::ComplectSpace)
                .collect::<Vec<_>>(),
            1 => EFFECTS
                .iter()
                .filter(|e| e.category == AutoDJEffectCategory::ComplectColor)
                .collect::<Vec<_>>(),
            2 => EFFECTS
                .iter()
                .filter(|e| e.category == AutoDJEffectCategory::ComplectSpace)
                .collect::<Vec<_>>(),
            3 => EFFECTS
                .iter()
                .filter(|e| e.category == AutoDJEffectCategory::SimplifyColor)
                .collect::<Vec<_>>(),
            4 => EFFECTS
                .iter()
                .filter(|e| {
                    e.category == AutoDJEffectCategory::SimplifySpace
                        || e.category == AutoDJEffectCategory::ComplectColor
                })
                .collect::<Vec<_>>(),
            5 => EFFECTS
                .iter()
                .filter(|e| e.category == AutoDJEffectCategory::SimplifySpace)
                .collect::<Vec<_>>(),
            _ => panic!("Don't know how to handle AutoDJ slot {}", ix),
        };
        let descriptor = descriptor_options.choose(&mut rng).unwrap();

        let target_intensity = rng.gen_range(descriptor.intensity_min..descriptor.intensity_max);

        let frequency = if descriptor.random_frequency {
            RANDOM_FREQUENCY_OPTIONS.choose(&mut rng).unwrap().clone()
        } else {
            None
        };

        let id = NodeId::gen();
        let props = NodeProps::EffectNode(EffectNodeProps {
            name: descriptor.name.to_owned(),
            frequency,
            ..Default::default()
        });
        let effect = AutoDJEffect {
            id,
            target_intensity,
        };
        (props, effect)
    }

    fn stable_timer() -> usize {
        let mut rng = rand::thread_rng();
        rng.gen_range(STABLE_TIMER_MIN..STABLE_TIMER_MAX)
    }

    pub fn update(&mut self, props: &mut Props) {
        match self.try_update(props) {
            Some(_) => {}
            None => {
                self.state = AutoDJState::Broken;
            }
        }
    }

    fn try_update(&mut self, props: &mut Props) -> Option<()> {
        fn check_integrity<const N: usize>(
            left_placeholder: NodeId,
            right_placeholder: NodeId,
            props: &Props,
            effects: &[AutoDJEffect; N],
        ) -> Option<()> {
            // Check the integrity of the Auto DJ
            // We expect N + 2 nodes and N + 1 edges to exist.

            let ensure_nodes = &[left_placeholder, right_placeholder];
            for node_id in ensure_nodes.iter().chain(effects.iter().map(|e| &e.id)) {
                props.graph.nodes.iter().find(|&n| n == node_id)?;
                props.node_props.get(node_id)?;
            }

            for i in 0..N + 1 {
                let from = if i == 0 {
                    left_placeholder
                } else {
                    effects[i - 1].id
                };
                let to = if i == N {
                    right_placeholder
                } else {
                    effects[i].id
                };
                props
                    .graph
                    .edges
                    .iter()
                    .find(|&e| e == &Edge { from, to, input: 0 })?;
            }

            Some(())
        }

        match &mut self.state {
            AutoDJState::Pending => {
                let left_placeholder = NodeId::gen();
                let right_placeholder = NodeId::gen();
                props.node_props.insert(
                    left_placeholder,
                    NodeProps::PlaceholderNode(PlaceholderNodeProps {}),
                );
                props.graph.nodes.push(left_placeholder);
                let mut prev_id = left_placeholder;
                let effects: [_; STABLE_EFFECT_COUNT] = (0..STABLE_EFFECT_COUNT)
                    .map(|i| {
                        let (mut new_props, effect) = Self::pick_new_node(i);
                        if let NodeProps::EffectNode(e) = &mut new_props {
                            e.intensity = Some(effect.target_intensity); // Initialize nodes directly to their target intensity
                        }
                        props.node_props.insert(effect.id, new_props);
                        props.graph.nodes.push(effect.id);
                        props.graph.edges.push(Edge {
                            from: prev_id,
                            to: effect.id,
                            input: 0,
                        });
                        prev_id = effect.id;
                        effect
                    })
                    .collect::<Vec<_>>()
                    .try_into()
                    .unwrap();
                props.node_props.insert(
                    right_placeholder,
                    NodeProps::PlaceholderNode(PlaceholderNodeProps {}),
                );
                props.graph.nodes.push(right_placeholder);
                props.graph.edges.push(Edge {
                    from: prev_id,
                    to: right_placeholder,
                    input: 0,
                });
                self.state = AutoDJState::Running(AutoDJRunning {
                    left_placeholder,
                    right_placeholder,
                    action: AutoDJAction::Stable(AutoDJStable {
                        effects,
                        timer: Self::stable_timer(),
                    }),
                });
            }
            AutoDJState::Running(running_state) => {
                match &mut running_state.action {
                    AutoDJAction::Stable(stable_state) => {
                        check_integrity(
                            running_state.left_placeholder,
                            running_state.right_placeholder,
                            props,
                            &stable_state.effects,
                        )?;

                        // Decrement the stable timer
                        // If we have hit 0, transition to crossfade
                        if stable_state.timer > 0 {
                            stable_state.timer -= 1;
                        } else {
                            let mut rng = rand::thread_rng();

                            // Pick a node at random to fade out & replace
                            let fade_out_ix = rng.gen_range(0..STABLE_EFFECT_COUNT);
                            let (new_props, new_effect) = Self::pick_new_node(fade_out_ix);

                            // Randomly insert before or after the node we are replacing
                            let fade_in_ix = rng.gen_range(fade_out_ix..=fade_out_ix + 1);

                            // Remove edge from graph that inputs to fade_in_ix
                            let from = if fade_in_ix > 0 {
                                stable_state.effects[fade_in_ix - 1].id
                            } else {
                                running_state.left_placeholder
                            };

                            let to = if fade_in_ix < STABLE_EFFECT_COUNT {
                                stable_state.effects[fade_in_ix].id
                            } else {
                                running_state.right_placeholder
                            };

                            let delete_edge = Edge { from, to, input: 0 };
                            props.graph.edges.retain(|e| e != &delete_edge);

                            let mut effects: [_; STABLE_EFFECT_COUNT + 1] = (0
                                ..STABLE_EFFECT_COUNT + 1)
                                .map(|i| {
                                    if i < fade_in_ix {
                                        stable_state.effects[i].clone()
                                    } else if i == fade_in_ix {
                                        new_effect.clone()
                                    } else {
                                        stable_state.effects[i - 1].clone()
                                    }
                                })
                                .collect::<Vec<_>>()
                                .try_into()
                                .unwrap();

                            // Apply correction to fade_out_ix so it points to the right index in the new array
                            let fade_out_ix = if fade_in_ix <= fade_out_ix {
                                fade_out_ix + 1
                            } else {
                                fade_out_ix
                            };

                            // Take the current (possibly altered) intensity value of the "fade out" effect as its target
                            let fade_out_props = props.node_props.get(&effects[fade_out_ix].id)?;
                            if let NodeProps::EffectNode(p) = fade_out_props {
                                if let Some(i) = p.intensity {
                                    effects[fade_out_ix].target_intensity = i;
                                }
                            }

                            // Add new node
                            props.node_props.insert(new_effect.id, new_props);
                            props.graph.nodes.push(new_effect.id);

                            // Add new left edge
                            let from = if fade_in_ix > 0 {
                                effects[fade_in_ix - 1].id
                            } else {
                                running_state.left_placeholder
                            };
                            props.graph.edges.push(Edge {
                                from,
                                to: new_effect.id,
                                input: 0,
                            });

                            // Add new right edge
                            let to = if fade_in_ix < STABLE_EFFECT_COUNT + 1 - 1 {
                                effects[fade_in_ix + 1].id
                            } else {
                                running_state.right_placeholder
                            };
                            props.graph.edges.push(Edge {
                                from: new_effect.id,
                                to,
                                input: 0,
                            });

                            // Transition state to crossfade
                            running_state.action = AutoDJAction::Crossfade(AutoDJCrossfade {
                                effects,
                                fade_in_ix,
                                fade_out_ix,
                                start_timer: CROSSFADE_TIMER,
                                timer: CROSSFADE_TIMER,
                            });
                        }
                    }
                    AutoDJAction::Crossfade(crossfade_state) => {
                        check_integrity(
                            running_state.left_placeholder,
                            running_state.right_placeholder,
                            props,
                            &crossfade_state.effects,
                        )?;

                        // Decrement the stable timer
                        // If we have hit 0, transition to stable
                        if crossfade_state.timer > 0 {
                            crossfade_state.timer -= 1;
                            let alpha =
                                crossfade_state.timer as f32 / crossfade_state.start_timer as f32;
                            let fade_in_effect =
                                &crossfade_state.effects[crossfade_state.fade_in_ix];
                            let mut fade_in_props = props.node_props.get_mut(&fade_in_effect.id)?;
                            if let NodeProps::EffectNode(e) = &mut fade_in_props {
                                e.intensity = Some(fade_in_effect.target_intensity * (1. - alpha));
                            }
                            let fade_out_effect =
                                &crossfade_state.effects[crossfade_state.fade_out_ix];
                            let mut fade_out_props =
                                props.node_props.get_mut(&fade_out_effect.id)?;
                            if let NodeProps::EffectNode(e) = &mut fade_out_props {
                                e.intensity = Some(fade_out_effect.target_intensity * alpha);
                            }
                        } else {
                            let remove_id = crossfade_state.effects[crossfade_state.fade_out_ix].id;

                            // Remove node at fade_out_ix
                            props.node_props.remove(&remove_id);
                            props.graph.nodes.retain(|n| n != &remove_id);

                            // Remove both edges that connect to fade_out_ix
                            let from = if crossfade_state.fade_out_ix > 0 {
                                crossfade_state.effects[crossfade_state.fade_out_ix - 1].id
                            } else {
                                running_state.left_placeholder
                            };
                            let delete_left_edge = Edge {
                                from,
                                to: remove_id,
                                input: 0,
                            };
                            let to = if crossfade_state.fade_out_ix < STABLE_EFFECT_COUNT + 1 - 1 {
                                crossfade_state.effects[crossfade_state.fade_out_ix + 1].id
                            } else {
                                running_state.right_placeholder
                            };
                            let delete_right_edge = Edge {
                                from: remove_id,
                                to,
                                input: 0,
                            };
                            props
                                .graph
                                .edges
                                .retain(|e| e != &delete_left_edge && e != &delete_right_edge);

                            // Add bypass edge
                            props.graph.edges.push(Edge { from, to, input: 0 });

                            // Remove node from AutoDJ effects list
                            let effects = crossfade_state
                                .effects
                                .iter()
                                .enumerate()
                                .filter_map(|(i, e)| {
                                    if i == crossfade_state.fade_out_ix {
                                        None
                                    } else {
                                        Some(e.clone())
                                    }
                                })
                                .collect::<Vec<_>>()
                                .try_into()
                                .unwrap();

                            // Transition state to stable
                            running_state.action = AutoDJAction::Stable(AutoDJStable {
                                effects,
                                timer: Self::stable_timer(),
                            });
                        }
                    }
                }
            }
            AutoDJState::Broken => {}
        }
        Some(())
    }

    pub fn is_broken(&self) -> bool {
        matches!(self.state, AutoDJState::Broken)
    }
}
