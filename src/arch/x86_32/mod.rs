extern crate multiboot;

use axel_context;
use core::mem;
use core::slice;
use graphic;
use graphic::*;

pub fn init(argv: &[usize])
{
    // let x = 640;
    // let y = 480;
    // for i in 0..(x * y){
    //     let addr = 0xFD000000 + (i * 4);
    //     // unsafe {*(addr as *mut u32) = 0xFF000000}; // rsvd
    //     // unsafe {*(addr as *mut u32) = 0x00FF0000}; // R
    //     // unsafe {*(addr as *mut u32) = 0x0000FF00}; // G
    //     // unsafe {*(addr as *mut u32) = 0x000000FF}; // B
    //     unsafe {*(addr as *mut u32) = 0x00D7003A};
    // }
    {
        let BACKGROUND_COLOR = graphic::Color::Code(0xD7003A);
        let bit_info = graphic::ColorBitInfo {
            size_r: 8,
            size_g: 8,
            size_b: 8,
            size_rsvd: 8,
            pos_r: 16,
            pos_g: 8,
            pos_b: 0,
            pos_rsvd: 24,
        };
        let mut display = graphic::GraphicalDisplay::new(0xFD000000, graphic::Position(640, 480), bit_info);
        display.fill_display(&BACKGROUND_COLOR);

        let base_x = 0;
        let base_y = 480 - 27;
        display.fill_area_by_size(Position(0, base_y), Position(640, 480), &Color::Code(0xC6C6C6));
        display.fill_area_by_size(Position( 3,  3 + base_y), Position(60, 1), &Color::Code(0xFFFFFF));  // above edge.
        display.fill_area_by_size(Position( 3, 23 + base_y), Position(60, 2), &Color::Code(0x848484));  // below edge.
        display.fill_area_by_size(Position( 3,  3 + base_y), Position(1, 20), &Color::Code(0xFFFFFF));  // left edge.
        display.fill_area_by_size(Position(61,  4 + base_y), Position(1, 20), &Color::Code(0x848484));  // right edge.
        display.fill_area_by_size(Position(62,  4 + base_y), Position(1, 20), &Color::Code(0x000001));  // right edge.
    }

    // for i in (x * y - x)..(x * y){
    //    let addr = 0xFD000000 + (i * 4);
    //    unsafe {*(addr as *mut u32) = 0x00FF0000};
    // }

    // {
    //     const TEXT_MODE_VRAM_ADDR: usize = 0xB8000;
    //     const TEXT_MODE_WIDTH: usize     = 80;
    //     const TEXT_MODE_HEIGHT: usize    = 25;
    //     let mut display = graphic::CharacterDisplay::new(TEXT_MODE_VRAM_ADDR, graphic::Position(TEXT_MODE_WIDTH, TEXT_MODE_HEIGHT));
    //     display.clear_screen();
    //     unsafe { axel_context::AXEL_CONTEXT.kernel_output_device = Some(display); }
    //     println!("Start Axel.");
    // }

    let mboot = unsafe {
        let multiboot_info_addr = argv[0] as multiboot::PAddr;
        let wraped = multiboot::Multiboot::new(multiboot_info_addr, paddr_to_slice);
        match wraped {
            None => panic!("No multiboot info."),
            Some(mboot) => mboot,
        }
    };

    println!("HLT");
    loop {
        unsafe {
            asm!("hlt");
        }
    }
}


//  Translate a physical memory address and size into a slice
pub fn paddr_to_slice<'a>(ptr_addr: multiboot::PAddr, sz: usize) -> Option<&'a [u8]>
{
    unsafe {
        // TODO
        // let ptr = mem::transmute(p + KERNEL_BASE);
        let ptr = mem::transmute(ptr_addr as usize);
        Some(slice::from_raw_parts(ptr,  sz))
    }
}
