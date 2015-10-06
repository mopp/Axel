#![feature(lang_items)]
#![feature(start)]
#![no_main]
#![feature(no_std)]
#![no_std]
#![feature(asm)]

#[no_mangle]
#[start]
#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
#[cfg(not(test))]
pub extern fn main()
{
    let mut i = 0xB8000;
    while i < 0xC0000 {
        unsafe {
            *(i as *mut u16) = 0x1220;
        }
        i += 2;
    }
    loop {
        unsafe {
            asm!("hlt");
        }
    }
}

pub fn return_two() -> u16 {
    2
}


#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn return_two_test() {
        let x = return_two();
        assert!(x == 2);
    }
}


#[cfg(not(test))]
#[lang = "stack_exhausted"]
extern fn stack_exhausted() {}

#[cfg(not(test))]
#[lang = "eh_personality"]
extern fn eh_personality() {}

#[cfg(not(test))]
#[lang = "panic_fmt"]
pub fn panic_fmt(_: &core::fmt::Arguments, _: &(&'static str, usize)) -> !
{
    loop { }
}
