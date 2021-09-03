#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
#[inline(always)]
pub fn cpu_yield() {
    unsafe { asm!("pause") };
}
