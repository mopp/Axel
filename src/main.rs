#![feature(asm)]
#![feature(lang_items)]
#![feature(no_std)]
#![feature(start)]
#![no_main]
#![no_std]


extern crate multiboot;
use core::mem;
use core::slice;
use multiboot::*;

mod graphic;


#[no_mangle]
#[start]
pub extern fn main(multiboot_info_addr: PAddr)
{
    let mboot;
    unsafe {
        mboot = Multiboot::new(multiboot_info_addr, paddr_to_slice);
    }
    // let display  =  graphic::CharacterDisplay::new(0xB8000, graphic::Position(800, 640));

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

//  Translate a physical memory address and size into a slice
pub unsafe fn paddr_to_slice<'a>(p: PAddr, sz: usize) -> Option<&'a [u8]>
{
    // TODO
    // let ptr = mem::transmute(p + KERNEL_BASE);
    let ptr = mem::transmute(0);
    Some(slice::from_raw_parts(ptr,  sz))
}


#[lang = "stack_exhausted"]
extern fn stack_exhausted() {}


#[lang = "eh_personality"]
extern fn eh_personality() {}


#[lang = "panic_fmt"]
pub fn panic_fmt(_: &core::fmt::Arguments, _: &(&'static str, usize)) -> !
{
    loop {}
}
