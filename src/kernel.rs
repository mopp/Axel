#![cfg_attr(test, feature(allocator_api))]
#![feature(asm)]
#![feature(lang_items)]
#![feature(panic_info_message)]
#![feature(ptr_internals)]
#![feature(ptr_wrapping_offset_from)]
#![feature(start)]
#![no_std]

#[cfg(test)]
#[macro_use]
extern crate std;
#[macro_use]
extern crate bitflags;
#[macro_use]
extern crate lazy_static;
extern crate multiboot2;
extern crate rlibc;
extern crate spin;
#[macro_use]
extern crate failure;

#[cfg(not(test))]
#[macro_use]
mod log;
mod arch;
mod context;
mod graphic;
mod list;
mod memory;

use arch::Initialize;
use core::panic::PanicInfo;
use memory::address::VirtualAddress;

#[cfg(not(test))]
#[start]
#[no_mangle]
pub extern "C" fn main(argc: usize, argv: *const VirtualAddress) {
    memory::clean_bss_section();

    let argv: &[VirtualAddress] = unsafe { core::slice::from_raw_parts(argv, argc) };
    if let Err(e) = arch::Initializer::init(argv) {
        panic!("arch::init fails: {}", e);
    }
}

#[cfg(not(test))]
#[lang = "eh_personality"]
pub extern "C" fn eh_personality() {}

#[cfg(not(test))]
#[lang = "panic_impl"]
#[no_mangle]
pub extern "C" fn panic_impl(pi: &PanicInfo) -> ! {
    println!("Panic !");
    if let Some(msg) = pi.message() {
        println!("  msg: {}", msg);
    }
    if let Some(msg) = pi.message() {
        println!("  msg: {}", msg);
    }

    loop {}
}

#[no_mangle]
pub extern "C" fn abort() {
    println!("abort");
    loop {}
}
