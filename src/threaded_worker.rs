use crate::types::{WorkResult, WorkHandle};
use std::thread;
use std::thread::JoinHandle;
use std::sync::{Arc, Weak};
use std::sync::atomic::AtomicBool;

#[derive(Debug)]
pub struct ThreadWorkHandle<T> {
    handle: JoinHandle<T>,
    alive: Weak<AtomicBool>,
}

impl<T: Send + 'static> ThreadWorkHandle<T> {
    pub fn new(f: fn () -> T) -> Self {
        let alive = Arc::new(AtomicBool::new(true));
        let alive_weak = Arc::downgrade(&alive);

        let handle = thread::spawn(move || {
            let r = f();
            drop(alive); // Don't know if there's a better way to capture this value
            r
        });

        ThreadWorkHandle {
            alive: alive_weak,
            handle: handle,
        }
    }
}

impl<T: Send + 'static> WorkHandle for ThreadWorkHandle<T> {
    type Output = T;

    fn alive(&self) -> bool {
        match self.alive.upgrade() {
            Some(_) => true,
            None => false,
        }
    }

    fn join(self) -> WorkResult<T> {
        match self.handle.join() {
            thread::Result::Ok(v) => WorkResult::Ok(v),
            thread::Result::Err(_) => WorkResult::Err(()),
        }
    }
}

