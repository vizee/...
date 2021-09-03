use std::cell::UnsafeCell;
use std::future::Future;
use std::mem;
use std::pin::Pin;
use std::sync::atomic::{AtomicU8, Ordering};
use std::sync::Arc;
use std::task::{Context, Poll, Waker};

use crate::latch::Latch;

const STATE_IDLE: u8 = 0;
const STATE_SIGNALED: u8 = 1;
const STATE_WAITING: u8 = 2;

struct OneNotifyInner {
    state: AtomicU8,
    latch: Latch,
    waker: UnsafeCell<Option<Waker>>,
}

unsafe impl Sync for OneNotifyInner {}

pub struct OneNotifyTx(Arc<OneNotifyInner>);

impl OneNotifyTx {
    pub fn pulse(&mut self) {
        let inner = &*self.0;

        let mut state = inner.state.load(Ordering::Acquire);
        if state == STATE_IDLE {
            state = inner
                .state
                .compare_exchange(
                    STATE_IDLE,
                    STATE_SIGNALED,
                    Ordering::AcqRel,
                    Ordering::Acquire,
                )
                .unwrap_or_else(|e| e);
        }

        // 如果在这里 state 变成 idle，那么说明它是从 signaled 到 idle

        if state == STATE_WAITING {
            // 如果 state 在 waiting 状态，必然 waker 不为 None
            // 如果是 mpsc，需要检查 w 是否为 nil
            inner.latch.acquire();
            let w = mem::replace(unsafe { &mut *inner.waker.get() }, None);
            assert!(w.is_some(), "waker must be not none");
            // 在 latch 内消费 waiting 状态
            inner.state.store(STATE_IDLE, Ordering::Release);
            inner.latch.release();
            w.unwrap().wake();
        }
    }
}

pub struct OneNotifyRx(Arc<OneNotifyInner>);

impl Future for OneNotifyRx {
    type Output = ();

    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        let inner = &*self.0;

        if inner.state.load(Ordering::Acquire) != STATE_SIGNALED {
            inner.latch.acquire();
            // 在 latch 内再次获取状态，确认 waiting 状态没被消费
            // 如果这里 state 变成 idle，说明期间有 pulse
            let state = inner.state.load(Ordering::Acquire);
            if state != STATE_SIGNALED {
                if state == STATE_IDLE {
                    // 在 latch 内更新状态
                    inner.state.store(STATE_WAITING, Ordering::Release);
                }
                let waker = unsafe { &mut *inner.waker.get() };
                if !waker.as_ref().map_or(false, |w| w.will_wake(cx.waker())) {
                    *waker = Some(cx.waker().clone());
                }
            }
            inner.latch.release();

            if state != STATE_SIGNALED {
                return Poll::Pending;
            }
        }

        // 消费 signaled 状态
        inner.state.store(STATE_IDLE, Ordering::Release);
        Poll::Ready(())
    }
}

pub fn one_notify() -> (OneNotifyTx, OneNotifyRx) {
    let inner = Arc::new(OneNotifyInner {
        state: AtomicU8::new(STATE_IDLE),
        latch: Latch::new(),
        waker: UnsafeCell::new(None),
    });
    (OneNotifyTx(inner.clone()), OneNotifyRx(inner.clone()))
}

#[cfg(test)]
mod tests {
    use std::pin::Pin;
    use std::time::Instant;

    use tokio::time::{self};

    use super::*;

    #[tokio::test]
    async fn test_one_notify() {
        let (mut tx, mut rx) = one_notify();
        let now = Instant::now();

        tokio::spawn(async move {
            println!("rx| start: {:?}", now.elapsed());
            loop {
                Pin::new(&mut rx).await;
                println!("rx| recv: {:?}", now.elapsed());
            }
        });

        tokio::spawn(async move {
            for _ in 0..8 {
                time::sleep(time::Duration::from_secs(1)).await;
                tx.pulse();
                println!("tx| shot: {:?}", now.elapsed());
            }
        });

        time::sleep(time::Duration::from_secs(10)).await;
    }
}
