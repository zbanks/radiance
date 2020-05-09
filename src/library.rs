use crate::err::{Error, JsResult, Result};

use log::*;
use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;
use wasm_bindgen::JsCast;
use wasm_bindgen_futures::{spawn_local, JsFuture};
use web_sys::{Request, RequestInit, RequestMode, Response};

#[derive(Clone)]
pub struct Library {
    library_ref: Rc<RefCell<LibraryRef>>,
}

struct LibraryRef {
    effects: HashMap<String, Status>,
}

#[derive(Clone)]
pub enum Status {
    Pending,
    Done(String),
    Failed,
    Invalid,
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
            effects: Default::default(),
        };

        Library {
            library_ref: Rc::new(RefCell::new(library_ref)),
        }
    }

    pub fn load_all(&self) {
        let lib = Rc::clone(&self.library_ref);
        spawn_local(async move {
            let list_text = fetch_url("/effect_list.txt").await.unwrap();
            info!("{}", list_text);
            let futs = list_text
                .lines()
                .map(|name| Self::load_effect(Rc::clone(&lib), name));
            futures::future::join_all(futs).await;
        });
    }

    async fn load_effect(lib: Rc<RefCell<LibraryRef>>, effect_name: &str) {
        lib.borrow_mut()
            .effects
            .insert(effect_name.to_string(), Status::Pending);
        async move {
            let url = format!("/effects/{}.glsl", effect_name);
            let text = match fetch_url(&url).await {
                Ok(source) => Status::Done(source),
                Err(e) => {
                    error!("Failed to fetch {}: {}", url, e);
                    Status::Failed
                }
            };
            lib.borrow_mut()
                .effects
                .insert(effect_name.to_string(), text);
        }
        .await
    }

    pub fn effect_source(&self, effect_name: &str) -> Status {
        let lib = self.library_ref.borrow();
        (*lib)
            .effects
            .get(effect_name)
            .cloned()
            .unwrap_or(Status::Invalid)
    }
}
