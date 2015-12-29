use super::super::graphic;
use graphic::Display;

extern crate multiboot;
use self::multiboot::PAddr;

use core::mem;
use core::slice;
use core::fmt::Write;

pub fn init(argc: usize, argv: *const usize)
{
    let multiboot_info_addr;
    unsafe {
        let argv = slice::from_raw_parts(argv, argc);
        multiboot_info_addr = argv[0] as multiboot::PAddr;
    }

    const TEXT_MODE_VRAM_ADDR: usize = 0xB8000;
    const TEXT_MODE_WIDTH: usize     = 80;
    const TEXT_MODE_HEIGHT: usize    = 25;

    let mut display = graphic::CharacterDisplay::new(TEXT_MODE_VRAM_ADDR, graphic::Position(TEXT_MODE_WIDTH, TEXT_MODE_HEIGHT));
    display.clear_screen();
    display.println("Start Axel.");
    writeln!(display, "0x{:x}", argc).unwrap();
    writeln!(display, "0x{:x}", argv as usize).unwrap();
    writeln!(display, "0x{:x}", multiboot_info_addr).unwrap();

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


//  Translate a physical memory address and size into a slice
pub unsafe fn paddr_to_slice<'a>(ptr_addr: PAddr, sz: usize) -> Option<&'a [u8]>
{
    // TODO
    // let ptr = mem::transmute(p + KERNEL_BASE);
    let ptr = mem::transmute(ptr_addr as usize);
    Some(slice::from_raw_parts(ptr,  sz))
}
