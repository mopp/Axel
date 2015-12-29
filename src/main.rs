#![feature(asm)]
#![feature(lang_items)]
#![feature(start)]
#![feature(core_intrinsics)]
// no_main is unused, maybe json setting file set os type is 'none'.
// #![no_main]
#![no_std]

mod arch;
mod graphic;

#[no_mangle]
#[start]
pub extern fn main(argc: usize, argv: *const usize)
{
    // Initialize stuffs depending on the architecture.
    arch::init_arch(argc, argv);
}


#[lang = "stack_exhausted"]
pub extern fn stack_exhausted() {}


#[lang = "eh_personality"]
pub extern fn eh_personality() {}


#[lang = "panic_fmt"]
pub extern fn panic_fmt(_: &core::fmt::Arguments, _: &(&'static str, usize)) -> !
{
    loop {}
}


#[no_mangle]
pub extern fn abort()
{
    loop {}
}


#[no_mangle]
pub unsafe extern fn memcpy(dest: *mut u8, src: *const u8, n: usize) -> *mut u8
{
    for i in 0..n {
        *dest.offset(i as isize) = *src.offset(i as isize);
    }
    return dest;
}


#[no_mangle]
pub unsafe extern fn __mulodi4() {}
