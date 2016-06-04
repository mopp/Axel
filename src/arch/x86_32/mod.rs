extern crate multiboot;
use self::multiboot::PAddr;

use axel_context;
use core::mem;
use core::slice;
use graphic;
use graphic::Display;
use log;

mod memory;

pub fn init(argc: usize, argv: *const usize)
{
    // let x = 640;
    // let y = 480;
    // for i in 0..(x * y - x){
        // let addr = 0xfd000000 + (i * 4);
        // unsafe {*(addr as *mut u32) = 0xFF0000};
    // }

    // for i in (x * y - x)..(x * y){
        // let addr = 0xfd000000 + (i * 4);
        // unsafe {*(addr as *mut u32) = 0x00FF00};
    // }

    {
        const TEXT_MODE_VRAM_ADDR: usize = 0xB8000;
        const TEXT_MODE_WIDTH: usize     = 80;
        const TEXT_MODE_HEIGHT: usize    = 25;
        let mut display = graphic::CharacterDisplay::new(TEXT_MODE_VRAM_ADDR, graphic::Position(TEXT_MODE_WIDTH, TEXT_MODE_HEIGHT));
        display.clear_screen();
        unsafe { axel_context::AXEL_CONTEXT.kernel_output_device = Some(display); }
        println!("Start Axel.");
        println!("VRAM 0x{:X}", TEXT_MODE_VRAM_ADDR);
    }

    let mboot = unsafe {
        let argv = slice::from_raw_parts(argv, argc);
        let multiboot_info_addr = argv[0] as multiboot::PAddr;
        let wraped = multiboot::Multiboot::new(multiboot_info_addr, paddr_to_slice);
        match wraped {
            None => panic!("No multiboot info."),
            Some(mboot) => mboot,
        }
    };

    memory::init(&mboot);

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
