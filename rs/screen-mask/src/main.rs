#![windows_subsystem = "windows"]

use std::fmt::Display;
use std::sync::Mutex;
use std::{mem, ptr};

use windows_sys::Win32::Foundation::*;
use windows_sys::Win32::Graphics::Gdi::*;
use windows_sys::Win32::System::LibraryLoader::GetModuleHandleW;
use windows_sys::Win32::UI::Controls::InitCommonControls;
use windows_sys::Win32::UI::Input::KeyboardAndMouse::*;
use windows_sys::Win32::UI::WindowsAndMessaging::*;
use windows_sys::core::*;

fn main() {
    unsafe { InitCommonControls() };

    if let Err(_) = init_window() {
        return;
    }

    let mut message = unsafe { mem::zeroed() };
    while unsafe { GetMessageW(&mut message, ptr::null_mut(), 0, 0) } != 0 {
        unsafe { DispatchMessageW(&message) };
    }
}

fn init_window() -> Result<(), Error> {
    const WND_WIDTH: i32 = 600;
    const WND_HEIGHT: i32 = 320;

    let h_instance = unsafe { GetModuleHandleW(ptr::null()) };
    debug_assert!(!h_instance.is_null());
    let class_name = w!("ScreenMask");
    let h_icon = unsafe { LoadIconW(ptr::null_mut(), IDI_APPLICATION) };
    let wnd_class = WNDCLASSEXW {
        cbSize: size_of::<WNDCLASSEXW>() as u32,
        style: CS_HREDRAW | CS_VREDRAW,
        lpfnWndProc: Some(wnd_proc),
        cbClsExtra: 0,
        cbWndExtra: 0,
        hInstance: h_instance,
        hIcon: h_icon,
        hCursor: unsafe { LoadCursorW(ptr::null_mut(), IDC_ARROW) },
        hbrBackground: unsafe { CreateSolidBrush(rgb(31, 31, 31)) },
        lpszMenuName: ptr::null(),
        lpszClassName: class_name,
        hIconSm: h_icon,
    };
    let atom = unsafe { RegisterClassExW(&wnd_class) };
    if atom == 0 {
        return Err(Error::os_last_error());
    }

    let hwnd_main = unsafe {
        let mut work_area: RECT = mem::zeroed();
        SystemParametersInfoW(SPI_GETWORKAREA, 0, (&mut work_area) as *mut _ as _, 0);
        CreateWindowExW(
            WS_EX_LAYERED,
            class_name,
            w!("Screen Mask"),
            WS_POPUP | WS_VISIBLE,
            work_area.left + (work_area.right - work_area.left) / 2 - WND_WIDTH / 2,
            work_area.top + (work_area.bottom - work_area.top) / 2 - WND_HEIGHT / 2,
            WND_WIDTH,
            WND_HEIGHT,
            ptr::null_mut(),
            ptr::null_mut(),
            h_instance,
            ptr::null_mut(),
        )
    };
    unsafe {
        SetLayeredWindowAttributes(hwnd_main, 0, 180, LWA_ALPHA);
        UpdateWindow(hwnd_main);
    }

    Ok(())
}

struct DraggingState {
    hwnd: usize,
    work_area: RECT,
    start_pt: POINT,
}

static DRAGGING_STATE: Mutex<Option<DraggingState>> = Mutex::new(None);

struct MaskState {
    active_level: usize,
}

static MASK_SATE: Mutex<MaskState> = Mutex::new(unsafe { mem::zeroed() });

extern "system" fn wnd_proc(hwnd: HWND, message: u32, wparam: WPARAM, lparam: LPARAM) -> LRESULT {
    match message {
        WM_LBUTTONDOWN => {
            let mut work_area: RECT = unsafe { mem::zeroed() };
            unsafe {
                SystemParametersInfoW(SPI_GETWORKAREA, 0, (&mut work_area) as *mut _ as _, 0);
            }
            let mut dragging_state = DRAGGING_STATE.lock().unwrap();
            *dragging_state = Some(DraggingState {
                hwnd: hwnd as usize,
                work_area,
                start_pt: POINT {
                    x: loword(lparam as u32) as i32,
                    y: hiword(lparam as u32) as i32,
                },
            });
            unsafe { SetCapture(hwnd) };
            0
        }
        WM_LBUTTONUP => {
            let mut dragging_state = DRAGGING_STATE.lock().unwrap();
            if dragging_state.is_some() {
                *dragging_state = None;
                unsafe { ReleaseCapture() };
            }
            0
        }
        WM_MOUSEMOVE => {
            if let Some(dragging_state) = DRAGGING_STATE.lock().unwrap().as_mut() {
                if dragging_state.hwnd == hwnd as usize {
                    let mut start_pt = dragging_state.start_pt;
                    let mut curr_pt = POINT {
                        x: loword(lparam as u32) as i32,
                        y: hiword(lparam as u32) as i32,
                    };
                    unsafe {
                        ClientToScreen(hwnd, &mut start_pt);
                        ClientToScreen(hwnd, &mut curr_pt);

                        let mut wnd_rect = mem::zeroed();
                        GetWindowRect(hwnd, &mut wnd_rect);
                        let width = wnd_rect.right - wnd_rect.left;
                        let height = wnd_rect.bottom - wnd_rect.top;
                        MoveWindow(
                            hwnd,
                            (wnd_rect.left + curr_pt.x - start_pt.x).clamp(
                                dragging_state.work_area.left,
                                dragging_state.work_area.right - width,
                            ),
                            (wnd_rect.top + curr_pt.y - start_pt.y).clamp(
                                dragging_state.work_area.top,
                                dragging_state.work_area.bottom - height,
                            ),
                            width,
                            height,
                            TRUE,
                        );
                        ScreenToClient(hwnd, &mut curr_pt);
                    }
                    dragging_state.start_pt = curr_pt;
                }
            }
            0
        }
        WM_KEYDOWN => {
            if wparam == VK_Q as usize {
                if (unsafe { GetKeyState(VK_CONTROL as i32) as u32 } & 0x8000) != 0 {
                    unsafe { DestroyWindow(hwnd) };
                }
            } else if wparam == VK_T as usize {
                if (unsafe { GetKeyState(VK_CONTROL as i32) as u32 } & 0x8000) != 0 {
                    let mut mask_state = MASK_SATE.lock().unwrap();

                    mask_state.active_level += 1;
                    if mask_state.active_level > 3 {
                        mask_state.active_level = 0
                    }
                    match mask_state.active_level {
                        0 => unsafe {
                            SetLayeredWindowAttributes(hwnd, 0, 180, LWA_ALPHA);
                            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                        },
                        n => unsafe {
                            SetLayeredWindowAttributes(hwnd, 0, 225 + (n as u8) * 10, LWA_ALPHA);
                            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                        },
                    }
                }
            } else if wparam == VK_OEM_PLUS as usize || wparam == VK_OEM_MINUS as usize {
                let is_add = wparam == VK_OEM_PLUS as usize;
                unsafe {
                    let mut wnd_rect = mem::zeroed();
                    GetWindowRect(hwnd, &mut wnd_rect);
                    let change = if is_add { 10 } else { -10 };
                    MoveWindow(
                        hwnd,
                        wnd_rect.left,
                        wnd_rect.top,
                        wnd_rect.right - wnd_rect.left,
                        (wnd_rect.bottom - wnd_rect.top + change).max(10),
                        TRUE,
                    );
                }
            }
            0
        }
        WM_DESTROY => {
            unsafe { PostQuitMessage(0) };
            0
        }
        _ => unsafe { DefWindowProcW(hwnd, message, wparam, lparam) },
    }
}

#[derive(Debug)]
enum Error {
    Os(u32),
}

impl Error {
    fn os_last_error() -> Self {
        Self::Os(unsafe { GetLastError() })
    }
}

impl Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            &Error::Os(code) => f.write_str(&code.to_string()),
        }
    }
}

#[inline]
pub const fn rgb(r: u8, g: u8, b: u8) -> u32 {
    ((r as u32) | ((g as u32) << 8) | ((b as u32) << 16)) as u32
}
#[inline]
pub const fn loword(value: u32) -> i16 {
    (value & 0xFFFF) as i16
}

#[inline]
pub const fn hiword(value: u32) -> i16 {
    ((value >> 16) & 0xFFFF) as i16
}
