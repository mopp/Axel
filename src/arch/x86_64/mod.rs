pub fn init(argv: &[usize])
{
    loop {
        unsafe {
            use axel_context;
            use core::mem;
            use core::slice;
            use graphic;
            use graphic::*;

            const TEXT_MODE_VRAM_ADDR: usize = 0xB8000 + 0x40000000;
            const TEXT_MODE_WIDTH: usize     = 80;
            const TEXT_MODE_HEIGHT: usize    = 25;
            let mut display = graphic::CharacterDisplay::new(TEXT_MODE_VRAM_ADDR, graphic::Position(TEXT_MODE_WIDTH, TEXT_MODE_HEIGHT));
            display.clear_screen();
            unsafe { axel_context::AXEL_CONTEXT.kernel_output_device = Some(display); }
            println!("Start Axel.");

            asm!("mov rax, $0
                  mov rbx, $1
                  mov rcx, 0xFFF
                 "
                 :
                 : "r"(argv.len()), "r"(argv[0])
                 : "rax", "rbx", "rcx"
                 : "intel", "volatile"
                );
            asm!("hlt");
        }
    }
}
