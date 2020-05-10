use crate::err::{Error, Result};

use log::*;
use serde::{Deserialize, Serialize};
use serde_json::Value as JsonValue;
use std::cell::RefCell;
use std::collections::hash_map::DefaultHasher;
use std::collections::HashMap;
use std::hash::{Hash, Hasher};
use std::rc::Rc;
use wasm_bindgen::JsCast;
use wasm_bindgen_futures::{spawn_local, JsFuture};
use web_sys::{Request, RequestInit, RequestMode, Response};

#[derive(Clone)]
pub struct Library {
    library_ref: Rc<RefCell<LibraryRef>>,
}

struct LibraryRef {
    items: HashMap<String, Status<Entry>>,
    content: Content,
}

#[serde(rename_all = "camelCase", tag = "type")]
#[derive(Clone, Serialize)]
enum Entry {
    Effect { source_hash: ContentHash },
}

#[derive(Clone)]
#[serde(rename_all = "camelCase", tag = "status")]
#[derive(Serialize)]
pub enum Status<T> {
    Pending,
    Loaded(T),
    Failed(String),
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

    // Convert this other `Promise` into a rust `Future`.
    let text = JsFuture::from(resp.text()?).await?.as_string().unwrap();

    Ok(text)
}

impl Library {
    pub fn new() -> Library {
        let library_ref = LibraryRef {
            items: Default::default(),
            content: Default::default(),
        };

        Library {
            library_ref: Rc::new(RefCell::new(library_ref)),
        }
    }

    pub fn load_all(&self) {
        let lib = Rc::clone(&self.library_ref);
        spawn_local(async move {
            let list_text = fetch_url("./effect_list.txt").await.unwrap();
            info!("{}", list_text);
            let futs = list_text
                .lines()
                .map(|name| Self::load_effect(Rc::clone(&lib), name));
            futures::future::join_all(futs).await;
        });
    }

    async fn load_effect(lib: Rc<RefCell<LibraryRef>>, effect_name: &str) {
        lib.borrow_mut()
            .items
            .insert(effect_name.to_string(), Status::Pending);
        async move {
            let url = format!("./effects/{}.glsl", effect_name);
            let status = match fetch_url(&url).await {
                Ok(source) => {
                    let entry = Entry::Effect {
                        source_hash: lib.borrow_mut().content.insert(source),
                    };
                    Status::Loaded(entry)
                }
                Err(e) => {
                    error!("Failed to fetch {}: {}", url, e);
                    Status::Failed(e.to_string())
                }
            };
            lib.borrow_mut()
                .items
                .insert(effect_name.to_string(), status);
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
            Status::Loaded(Entry::Effect { ref source_hash }) => {
                if let Some(source) = lib.content.get(source_hash) {
                    Status::Loaded(source.clone())
                } else {
                    Status::Failed(format!("Invalid hash: {:?}", source_hash))
                }
            }
            Status::Pending => Status::Pending,
            Status::Failed(e) => Status::Failed(e),
            Status::Invalid => Status::Invalid,
        }
    }

    pub fn items(&self) -> JsonValue {
        let lib = self.library_ref.borrow();
        serde_json::to_value(&lib.items).unwrap_or(JsonValue::Null)
    }

    pub fn content(&self, hash: &ContentHash) -> Option<String> {
        let lib = self.library_ref.borrow();
        lib.content.get(hash).cloned()
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
