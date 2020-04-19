use std::error;
use std::fmt;

pub type Result<T> = std::result::Result<T, Error>;

#[derive(Debug)]
pub enum Error {
    //MissingFeature(String),
    Runtime(String),
    Glsl(String),
    Js(wasm_bindgen::JsValue),
}

impl Error {
    pub fn new(details: &str) -> Error {
        Error::Runtime(String::from(details))
    }

    pub fn glsl(details: String) -> Error {
        Error::Glsl(details)
    }

    /*
    pub fn new_missing_feature(name: &str) -> Error {
        Error::MissingFeature(String::from(name))
    }
    */
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            //Error::MissingFeature(ref name) => write!(f, "Missing required feature: {}", name),
            Error::Runtime(ref details) => write!(f, "Runtime Error: {}", details),
            Error::Glsl(ref details) => write!(f, "GLSL Error:\n{}", details),
            Error::Js(ref e) => write!(f, "Javascript Error: {:?}", e), // XXX use of Debug
        }
    }
}

impl error::Error for Error {
    fn source(&self) -> Option<&(dyn error::Error + 'static)> {
        match *self {
            //Error::MissingFeature(ref name) => None,
            Error::Runtime(_) => None,
            Error::Glsl(_) => None,
            Error::Js(_) => None, // XXX propagate JsValue errors?
        }
    }
}

impl From<wasm_bindgen::JsValue> for Error {
    fn from(err: wasm_bindgen::JsValue) -> Error {
        Error::Js(err)
    }
}

impl From<&str> for Error {
    fn from(err: &str) -> Error {
        Error::new(err)
    }
}
