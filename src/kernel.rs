#![feature(asm)]
#![feature(lang_items)]
#![feature(start)]
#![feature(shared)]
#![no_std]

#[cfg(test)]
#[macro_use]
extern crate std;

#[cfg(not(test))]
#[macro_use]
mod log;

mod alist;
mod arch;
mod axel_context;
mod graphic;


#[start]
#[no_mangle]
pub extern fn main(argc: usize, argv: *const usize)
{
    // Initialize stuffs depending on the architecture.
    arch::init_arch(argc, argv);
}


#[cfg(not(test))]
#[lang = "eh_personality"]
pub extern fn eh_personality() {}


#[cfg(not(test))]
#[lang = "panic_fmt"]
pub extern fn panic_fmt(_: &core::fmt::Arguments, _: &(&'static str, usize)) -> !
{
    println!("panic_fmt");
    loop {}
}


#[no_mangle]
pub extern fn abort()
{
    println!("abort");
    loop {}
}
