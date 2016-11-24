use graphic;
use graphic::Display;
use memory;
use multiboot2;


pub fn init(argv: &[usize])
{
    println!("Start Axel.");

    let memory_regions: [memory::Region; 5] = [Default::default(); 5];
    for (i, m) in memory_regions.iter().enumerate() {
        println!("String #{} is {}", i, m.size);
    }
    println!("size {}", memory_regions.len());

    let multiboot_info = unsafe { multiboot2:: load(argv[0]) };
    if let Some(memory_map_tag) = multiboot_info.memory_map_tag() {
        for memory_area in memory_map_tag.memory_areas() {
            println!("Base addr: 0x{:X}", memory_area.base_addr);
            println!("Length   : {}KB", memory_area.length / 1024);
        }
    }

    unsafe {
        asm!("mov rax, $0
              mov rbx, $1
              mov rcx, 0xFFF"
              :
              : "r"(argv.len()), "r"(argv[0])
              : "rax", "rbx", "rcx"
              : "intel", "volatile"
            );
    }
}


pub fn obtain_kernel_console() -> Option<graphic::CharacterDisplay<'static>>
{
    const TEXT_MODE_VRAM_ADDR: usize = 0xB8000 + 0x40000000;
    const TEXT_MODE_WIDTH: usize     = 80;
    const TEXT_MODE_HEIGHT: usize    = 25;
    let mut display = graphic::CharacterDisplay::new(TEXT_MODE_VRAM_ADDR, graphic::Position(TEXT_MODE_WIDTH, TEXT_MODE_HEIGHT));
    display.clear_screen();
    Some(display)
}
