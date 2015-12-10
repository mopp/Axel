#![feature(asm)]
#![feature(lang_items)]
#![feature(start)]
#![no_main]
#![no_std]

extern crate multiboot;
use multiboot::{PAddr, Multiboot};

use core::mem;
use core::slice;
use core::fmt::Write;

mod graphic;
use graphic::{Position, Display};

mod arch;
use arch::init_arch;


const TEXT_MODE_VRAM_ADDR: usize = 0xB8000;
const TEXT_MODE_WIDTH: usize     = 80;
const TEXT_MODE_HEIGHT: usize    = 25;


#[no_mangle]
#[start]
pub extern fn main(multiboot_info_addr: PAddr)
{
    let mut display = graphic::CharacterDisplay::new(TEXT_MODE_VRAM_ADDR, graphic::Position(TEXT_MODE_WIDTH, TEXT_MODE_HEIGHT));
    display.clear_screen();
    display.println("Start Axel.");

    let mboot = unsafe { Multiboot::new(multiboot_info_addr, paddr_to_slice) }.unwrap();

    // Initialize stuffs depending on the architecture.
    init_arch();

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


//  Translate a physical memory address and size into a slice
pub unsafe fn paddr_to_slice<'a>(ptr_addr: PAddr, sz: usize) -> Option<&'a [u8]>
{
    // TODO
    // let ptr = mem::transmute(p + KERNEL_BASE);
    let ptr = mem::transmute(ptr_addr);
    Some(slice::from_raw_parts(ptr,  sz))
}


#[lang = "stack_exhausted"]
pub extern fn stack_exhausted() {}


#[lang = "eh_personality"]
pub extern fn eh_personality() {}


#[lang = "panic_fmt"]
pub extern fn panic_fmt(_: &core::fmt::Arguments, _: &(&'static str, usize)) -> !
{
    let mut display = graphic::CharacterDisplay::new(TEXT_MODE_VRAM_ADDR, graphic::Position(TEXT_MODE_WIDTH, TEXT_MODE_HEIGHT));
    display.clear_screen();
    display.println("Fault.");
    loop {}
}
