use crate::err::{Error, Result};

use log::*;
use serde::{Deserialize, Serialize};
use serde_json::Value as JsonValue;
use std::cell::RefCell;
use std::collections::hash_map::DefaultHasher;
use std::collections::HashMap;
use std::hash::{Hash, Hasher};
use std::rc::Rc;
use wasm_bindgen::{JsCast, JsValue};
use wasm_bindgen_futures::{spawn_local, JsFuture};
use web_sys::{Request, RequestInit, RequestMode, Response};

#[derive(Clone)]
pub struct Library {
    library_ref: Rc<RefCell<LibraryRef>>,
}

struct LibraryRef {
    items: HashMap<String, Status<Entry>>,
    content: Content,
    changed: Option<js_sys::Function>,
}

#[serde(tag = "nodeType")]
#[derive(Clone, Serialize)]
enum Entry {
    #[serde(rename_all = "camelCase")]
    EffectNode {
        name: String,
        source_hash: ContentHash,
    },
    #[serde(rename_all = "camelCase")]
    OutputNode { name: String },
    #[serde(rename_all = "camelCase")]
    MediaNode { name: String },
}

#[derive(Clone)]
#[serde(rename_all = "camelCase", tag = "status")]
#[derive(Serialize)]
pub enum Status<T> {
    Pending,
    Loaded(T),
    Failed { error: String },
    Invalid,
}

#[derive(Debug, Clone, Hash, PartialEq, Eq, PartialOrd, Ord, Serialize, Deserialize)]
pub struct ContentHash(String);

/// Content-Addressable Storage
#[derive(Debug, Default)]
struct Content {
    map: HashMap<ContentHash, String>,
}

async fn fetch_url(url: &str) -> Result<String> {
    let mut opts = RequestInit::new();
    opts.method("GET");
    opts.mode(RequestMode::Cors);

    let request = Request::new_with_str_and_init(&url, &opts)?;

    let window = web_sys::window().unwrap();
    let resp_value = JsFuture::from(window.fetch_with_request(&request)).await?;

    // `resp_value` is a `Response` object.
    assert!(resp_value.is_instance_of::<Response>());
    let resp: Response = resp_value.dyn_into().unwrap();

    if !resp.ok() {
        return Err(Error::Http(resp.status_text()));
    }

    // Convert this other `Promise` into a rust `Future`.
    let text = JsFuture::from(resp.text()?).await?.as_string().unwrap();

    Ok(text)
}

impl Library {
    pub fn new() -> Library {
        let mut lib = LibraryRef {
            items: Default::default(),
            content: Default::default(),
            changed: Default::default(),
        };
        lib.items.insert(
            "media".to_string(),
            Status::Loaded(Entry::MediaNode {
                name: "media".to_string(),
            }),
        );
        lib.items.insert(
            "output".to_string(),
            Status::Loaded(Entry::OutputNode {
                name: "output".to_string(),
            }),
        );

        Library {
            library_ref: Rc::new(RefCell::new(lib)),
        }
    }

    pub fn load_all(&self) {
        let lib = Rc::clone(&self.library_ref);
        spawn_local(async move {
            match fetch_url("./effect_list.txt").await {
                Ok(list_text) => {
                    let futs = list_text
                        .lines()
                        .map(|name| Self::load_effect(Rc::clone(&lib), name));
                    futures::future::join_all(futs).await;
                    lib.borrow().changed_callback();
                }
                Err(e) => {
                    error!("Unable to fetch effects_list; no effects will load ({})", e);
                }
            }
        });
    }

    async fn load_effect(lib: Rc<RefCell<LibraryRef>>, effect_name: &str) {
        lib.borrow_mut()
            .items
            .insert(effect_name.to_string(), Status::Pending);
        async move {
            let url = format!("./effects/{}.glsl", effect_name);
            let effect_name = effect_name.to_string();
            match fetch_url(&url).await {
                Ok(source) => {
                    lib.borrow_mut().set_effect_source(effect_name, source);
                }
                Err(e) => {
                    lib.borrow_mut().items.insert(
                        effect_name,
                        Status::Failed {
                            error: e.to_string(),
                        },
                    );
                }
            }
        }
        .await
    }

    pub fn effect_source(&self, effect_name: &str) -> Status<String> {
        let lib = self.library_ref.borrow();
        let status = (*lib)
            .items
            .get(effect_name)
            .cloned()
            .unwrap_or(Status::Invalid);
        match status {
            Status::Loaded(Entry::EffectNode {
                ref source_hash, ..
            }) => {
                if let Some(source) = lib.content.get(source_hash) {
                    Status::Loaded(source.clone())
                } else {
                    Status::Failed {
                        error: format!("Invalid hash: {:?}", source_hash),
                    }
                }
            }
            Status::Loaded(_) => Status::Invalid,
            Status::Pending => Status::Pending,
            Status::Failed { error } => Status::Failed { error },
            Status::Invalid => Status::Invalid,
        }
    }

    pub fn set_effect_source(&self, name: String, source: String) {
        self.library_ref
            .borrow_mut()
            .set_effect_source(name, source);
        self.library_ref.borrow().changed_callback();
    }

    pub fn items(&self) -> JsonValue {
        self.library_ref.borrow().items()
    }

    pub fn content(&self, hash: &ContentHash) -> Option<String> {
        let lib = self.library_ref.borrow();
        lib.content.get(hash).cloned()
    }

    pub fn set_changed(&self, callback: js_sys::Function) {
        self.library_ref.borrow_mut().changed = Some(callback);
    }
}

impl LibraryRef {
    fn items(&self) -> JsonValue {
        serde_json::to_value(&self.items).unwrap()
    }

    fn set_effect_source(&mut self, name: String, source: String) {
        let hash = self.content.insert(source);
        let entry = Entry::EffectNode {
            name: name.clone(),
            source_hash: hash,
        };
        self.items.insert(name, Status::Loaded(entry));
    }

    fn changed_callback(&self) {
        let items = JsValue::from_serde(&self.items()).unwrap();
        if let Some(callback) = &self.changed {
            let _ = callback.call1(&JsValue::NULL, &items);
        }
    }
}

impl Content {
    fn get(&self, hash: &ContentHash) -> Option<&String> {
        self.map.get(hash)
    }

    fn insert(&mut self, content: String) -> ContentHash {
        let hash = Self::hash(&content);
        self.map
            .entry(hash.clone())
            .and_modify(|e| assert_eq!(e, &content))
            .or_insert(content);
        hash
    }

    fn hash(content: &str) -> ContentHash {
        // TODO: Replace this with a more robust hash function
        let mut s = DefaultHasher::new();
        content.hash(&mut s);
        let h = s.finish();
        ContentHash(h.to_string())
    }
}
