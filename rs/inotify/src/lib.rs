#![feature(cstr_from_bytes_until_nul)]
#![feature(c_size_t)]
#![feature(core_ffi_c)]

use std::ffi::{c_int, CStr, CString};
use std::io::{Error, Result};
use std::mem::{size_of, MaybeUninit};
use std::os::unix::io::{AsRawFd, RawFd};
use std::path::Path;
use std::{slice, usize};

use tokio::io::unix::AsyncFd;
use tokio::io::Interest;

pub mod sys {
    use std::ffi::{c_int, c_size_t, c_ssize_t};

    pub const NAME_MAX: usize = 255;

    pub const ENOSPC: u32 = 28;

    pub const IN_NONBLOCK: c_int = 2048;
    pub const IN_CLOEXEC: c_int = 524288;

    pub const IN_ACCESS: u32 = 1;
    pub const IN_MODIFY: u32 = 2;
    pub const IN_ATTRIB: u32 = 4;
    pub const IN_CLOSE_WRITE: u32 = 8;
    pub const IN_CLOSE_NOWRITE: u32 = 16;
    pub const IN_OPEN: u32 = 32;
    pub const IN_MOVED_FROM: u32 = 64;
    pub const IN_MOVED_TO: u32 = 128;
    pub const IN_CREATE: u32 = 256;
    pub const IN_DELETE: u32 = 512;
    pub const IN_DELETE_SELF: u32 = 1024;
    pub const IN_MOVE_SELF: u32 = 2048;

    pub const IN_MOVE: u32 = IN_MOVED_FROM | IN_MOVED_TO;
    pub const IN_CLOSE: u32 = IN_CLOSE_WRITE | IN_CLOSE_NOWRITE;
    pub const IN_ALL_EVENTS: u32 = IN_ACCESS
        | IN_MODIFY
        | IN_ATTRIB
        | IN_CLOSE_WRITE
        | IN_CLOSE_NOWRITE
        | IN_OPEN
        | IN_MOVED_FROM
        | IN_MOVED_TO
        | IN_CREATE
        | IN_DELETE
        | IN_DELETE_SELF
        | IN_MOVE_SELF;

    pub const IN_UNMOUNT: u32 = 8192;
    pub const IN_Q_OVERFLOW: u32 = 16384;
    pub const IN_IGNORED: u32 = 32768;
    pub const IN_ONLYDIR: u32 = 16777216;
    pub const IN_DONT_FOLLOW: u32 = 33554432;
    pub const IN_EXCL_UNLINK: u32 = 67108864;
    pub const IN_MASK_ADD: u32 = 536870912;
    pub const IN_ISDIR: u32 = 1073741824;
    pub const IN_ONESHOT: u32 = 2147483648;

    #[repr(C)]
    pub struct InotifyEvent {
        pub wd: c_int,
        pub mask: u32,
        pub cookie: u32,
        pub len: u32,
    }

    extern "C" {
        fn __errno_location() -> *const c_int;

        pub fn inotify_init1(flags: c_int) -> c_int;
        pub fn inotify_add_watch(fd: c_int, pathname: *const i8, mask: u32) -> c_int;
        pub fn inotify_rm_watch(fd: c_int, wd: c_int) -> c_int;

        pub fn read(fd: c_int, buf: *mut u8, len: c_size_t) -> c_ssize_t;
        pub fn close(fd: c_int) -> c_int;
    }

    #[inline(always)]
    pub fn errno() -> c_int {
        unsafe { *__errno_location() }
    }
}

#[derive(Debug)]
pub struct InotifyEvent {
    pub wd: c_int,
    pub mask: u32,
    pub cookie: u32,
    pub name: String,
}

pub struct Wd<'a> {
    n: &'a Notifier,
    wd: c_int,
}

impl Drop for Wd<'_> {
    fn drop(&mut self) {
        self.n.inner.get_ref().rm_watch(self.wd).unwrap();
    }
}

pub struct Notifier {
    inner: AsyncFd<InotifyFd>,
}

impl Notifier {
    pub fn new() -> Result<Self> {
        Ok(Self {
            inner: AsyncFd::with_interest(InotifyFd::new(true)?, Interest::READABLE)?,
        })
    }

    pub fn watch<P: AsRef<Path>>(&self, path: P, mask: u32) -> Result<Wd<'_>> {
        use std::os::unix::ffi::OsStrExt;

        let p = path.as_ref().as_os_str();
        let wd = self
            .inner
            .get_ref()
            .add_watch(CString::new(p.as_bytes())?.as_c_str(), mask)?;

        Ok(Wd { wd: wd, n: self })
    }

    pub async fn read_events(&self) -> Result<Vec<InotifyEvent>> {
        let mut evs = Vec::new();
        loop {
            let mut guard = self.inner.readable().await?;
            match guard.try_io(|inner| inner.get_ref().read_event(|ev| evs.push(ev))) {
                Ok(res) => return res.map(|_| evs),
                Err(_would_block) => {}
            }
        }
    }
}

pub struct InotifyFd {
    fd: c_int,
}

impl Drop for InotifyFd {
    fn drop(&mut self) {
        unsafe { sys::close(self.fd) };
    }
}

impl InotifyFd {
    pub fn new(nonblock: bool) -> Result<Self> {
        let mut flags = sys::IN_CLOEXEC;
        if nonblock {
            flags |= sys::IN_NONBLOCK;
        }
        let fd = unsafe { sys::inotify_init1(flags) };
        if fd == -1 {
            Err(Error::from_raw_os_error(sys::errno()))
        } else {
            Ok(InotifyFd { fd })
        }
    }

    pub fn add_watch(&self, pathname: &CStr, mask: u32) -> Result<c_int> {
        let wd = unsafe { sys::inotify_add_watch(self.fd, pathname.as_ptr(), mask) };
        if wd == -1 {
            Err(Error::from_raw_os_error(sys::errno()))
        } else {
            Ok(wd)
        }
    }

    pub fn rm_watch(&self, wd: c_int) -> Result<()> {
        let ret = unsafe { sys::inotify_rm_watch(self.fd, wd) };
        if ret == 0 {
            Ok(())
        } else {
            Err(Error::from_raw_os_error(sys::errno()))
        }
    }

    pub fn read_event(&self, mut f: impl FnMut(InotifyEvent)) -> Result<()> {
        const C_INOTIFY_EVENT_SIZE: usize = size_of::<sys::InotifyEvent>();

        let mut buf = MaybeUninit::<[u8; C_INOTIFY_EVENT_SIZE + sys::NAME_MAX + 1]>::uninit();
        let buf = unsafe { buf.assume_init_mut() };
        let n = unsafe { sys::read(self.fd, buf as *mut _, buf.len()) };
        if n < 0 {
            return Err(Error::from_raw_os_error(sys::errno()));
        }

        let mut offs = 0isize;
        while offs < n {
            let ev =
                unsafe { &*((buf.as_ptr() as *const u8).offset(offs) as *const sys::InotifyEvent) };
            let name = if ev.len > 0 {
                unsafe {
                    CStr::from_bytes_until_nul(slice::from_raw_parts(
                        (buf.as_ptr() as *const u8).offset(offs + C_INOTIFY_EVENT_SIZE as isize),
                        ev.len as usize,
                    ))
                    .unwrap()
                }
            } else {
                unsafe { CStr::from_bytes_with_nul_unchecked(b"\0") }
            };

            f(InotifyEvent {
                wd: ev.wd,
                mask: ev.mask,
                cookie: ev.cookie,
                name: unsafe { std::str::from_utf8_unchecked(name.to_bytes()) }.to_string(),
            });
            offs += (C_INOTIFY_EVENT_SIZE + ev.len as usize) as isize;
        }
        Ok(())
    }
}

impl AsRawFd for InotifyFd {
    fn as_raw_fd(&self) -> RawFd {
        self.fd
    }
}

#[cfg(test)]
mod tests {
    use std::ffi::CString;

    use super::*;

    #[test]
    fn test_inotify_fd() {
        let ifd = InotifyFd::new(false).unwrap();
        let name = CString::new("/home/vizee/a").unwrap();
        let wd = ifd.add_watch(name.as_c_str(), sys::IN_ALL_EVENTS).unwrap();
        for _ in 0..10 {
            ifd.read_event(|e| println!("{:?}", e)).unwrap();
        }
        ifd.rm_watch(wd).unwrap()
    }
}
