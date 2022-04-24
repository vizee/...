use inotify::{sys, Notifier};

#[tokio::main]
async fn main() {
    let notifier = Notifier::new().unwrap();
    let _wd = notifier.watch(".", sys::IN_ALL_EVENTS).unwrap();
    let events = notifier.read_events().await.unwrap();
    for ev in events {
        println!("{:?}", ev);
    }
}
