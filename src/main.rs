#![feature(asm)]
#![feature(lang_items)]
#![feature(no_std)]
#![feature(start)]
#![no_main]
#![no_std]


extern crate multiboot;
use multiboot::*;
use multiboot::PAddr as PhysicalAddr;
use multiboot::VAddr as VirtualAddr;

mod graphic;


#[no_mangle]
#[start]
pub extern fn main(multiboot_info_addr: PhysicalAddr)
{
    let mboot = Multiboot::new(multiboot_info_addr, physical_addr_to_virtual_addr);
    let display = graphic::CharacterDisplay::new(0xB8000);

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


/// Translate a physical memory address into a kernel addressable location.
pub fn physical_addr_to_virtual_addr(p: PhysicalAddr) -> VirtualAddr {
    p as VirtualAddr
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
