use std::sync::atomic::{AtomicU8, Ordering};
use std::thread;

use crate::arch;

pub struct Latch(AtomicU8);

impl Latch {
    pub fn new() -> Latch {
        Latch(AtomicU8::new(0))
    }

    pub fn try_acquire(&self) -> bool {
        let curr = self.0.load(Ordering::Acquire);
        if curr == 0 {
            self.0
                .compare_exchange(0, 1, Ordering::SeqCst, Ordering::Relaxed)
                .is_ok()
        } else {
            false
        }
    }

    pub fn acquire(&self) {
        let mut c = 0;
        loop {
            if self.try_acquire() {
                return;
            }
            c += 1;
            if c <= 4 {
                arch::cpu_yield();
            } else {
                thread::yield_now();
            }
        }
    }

    pub fn release(&self) {
        self.0.store(0, Ordering::SeqCst);
    }
}
