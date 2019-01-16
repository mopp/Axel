#![feature(alloc_error_handler)]
#![feature(asm)]
#![feature(lang_items)]
#![feature(panic_info_message)]
#![feature(ptr_internals)]
#![feature(ptr_wrapping_offset_from)]
#![feature(allocator_api)]
#![feature(start)]
#![no_std]

#[cfg(test)]
#[macro_use]
extern crate std;
#[macro_use]
extern crate bitflags;
#[macro_use]
extern crate lazy_static;
#[macro_use]
extern crate intrusive_collections;
extern crate multiboot2;
extern crate rlibc;
extern crate spin;
#[macro_use]
extern crate failure;
extern crate x86_64;

#[cfg(not(test))]
#[macro_use]
mod log;
mod arch;
mod context;
mod graphic;
mod memory;

use self::arch::Initialize;
use self::memory::address::VirtualAddress;
use core::alloc::{GlobalAlloc, Layout};
use core::panic::PanicInfo;

#[cfg(not(test))]
#[start]
#[no_mangle]
pub extern "C" fn main(argc: usize, argv: *const VirtualAddress) {
    memory::clean_bss_section();

    let argv: &[VirtualAddress] = unsafe { core::slice::from_raw_parts(argv, argc) };
    println!("Start Axel");
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
    if let Some(location) = pi.location() {
        println!("panic occurred in file '{}' at line {}", location.file(), location.line());
    } else {
        println!("panic occurred but can't get location information...");
    }

    loop {}
}

#[no_mangle]
pub extern "C" fn abort() {
    println!("abort");
    loop {}
}

struct DummyAllocator;

unsafe impl GlobalAlloc for DummyAllocator {
    unsafe fn alloc(&self, _: Layout) -> *mut u8 {
        unimplemented!("");
    }

    unsafe fn dealloc(&self, _: *mut u8, _: Layout) {
        unimplemented!("");
    }
}

#[cfg(not(test))]
#[alloc_error_handler]
fn alloc_error(_: Layout) -> ! {
    unimplemented!("");
}

#[cfg(not(test))]
#[global_allocator]
static GLOBAL: DummyAllocator = DummyAllocator;
