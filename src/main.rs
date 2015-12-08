#![feature(asm)]
#![feature(lang_items)]
#![feature(start)]
#![no_main]
#![no_std]


extern crate multiboot;
use core::mem;
use core::slice;
use multiboot::*;

mod graphic;
use graphic::Display;

const TEXT_MODE_VRAM_ADDR: usize = 0xB8000;
const TEXT_MODE_WIDTH: usize     = 80;
const TEXT_MODE_HEIGHT: usize    = 25;

#[no_mangle]
#[start]
pub extern fn main(multiboot_info_addr: PAddr)
{
    let mboot = unsafe { Multiboot::new(multiboot_info_addr, paddr_to_slice) };
    let mut display = graphic::CharacterDisplay::new(TEXT_MODE_VRAM_ADDR, graphic::Position(TEXT_MODE_WIDTH, TEXT_MODE_HEIGHT));
    display.clear_screen();
    display.puts("Axel.");

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
pub extern fn stack_exhausted() {}


#[lang = "eh_personality"]
pub extern fn eh_personality() {}


#[lang = "panic_fmt"]
pub extern fn panic_fmt(_: &core::fmt::Arguments, _: &(&'static str, usize)) -> !
{
    loop {}
}
