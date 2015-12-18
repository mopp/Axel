#![feature(asm)]
#![feature(lang_items)]
#![feature(start)]
#![no_main]
#![no_std]

extern crate multiboot;
use multiboot::PAddr;

#[cfg(target_arch = "x86")]
mod graphic;
#[cfg(target_arch = "x86")]
use graphic::Display;

use core::mem;
use core::slice;
#[cfg(target_arch = "x86")]
use core::fmt::Write;

mod arch;
use arch::init_arch;


#[no_mangle]
#[start]
#[cfg(target_arch = "x86")]
pub extern fn main(multiboot_info_addr: PAddr)
{
    // Initialize stuffs depending on the architecture.
    init_arch();

    const TEXT_MODE_VRAM_ADDR: usize = 0xB8000;
    const TEXT_MODE_WIDTH: usize     = 80;
    const TEXT_MODE_HEIGHT: usize    = 25;

    let mut display = graphic::CharacterDisplay::new(TEXT_MODE_VRAM_ADDR, graphic::Position(TEXT_MODE_WIDTH, TEXT_MODE_HEIGHT));
    display.clear_screen();
    display.println("Start Axel.");

    let mboot = unsafe { multiboot::Multiboot::new(multiboot_info_addr, paddr_to_slice) }.unwrap();

    let lower = mboot.lower_memory_bound();
    let upper = mboot.upper_memory_bound();
    writeln!(display, "0x{:x}", lower.unwrap()).unwrap();
    writeln!(display, "0x{:x}", upper.unwrap()).unwrap();
    mboot.memory_regions().map(|regions| {
        for region in regions {
            writeln!(display, "Found 0x{}", region.length()).unwrap();
        }
    });

    loop {
        unsafe {
            asm!("hlt");
        }
    }
}


#[no_mangle]
#[start]
#[cfg(target_arch = "arm")]
pub extern fn main()
{
    // Initialize stuffs depending on the architecture.
    init_arch();

    loop {
        unsafe {
            asm!("mov r5, #55");
        }
    }
}


//  Translate a physical memory address and size into a slice
pub unsafe fn paddr_to_slice<'a>(ptr_addr: PAddr, sz: usize) -> Option<&'a [u8]>
{
    // TODO
    // let ptr = mem::transmute(p + KERNEL_BASE);
    let ptr = mem::transmute(ptr_addr as usize);
    Some(slice::from_raw_parts(ptr,  sz))
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
