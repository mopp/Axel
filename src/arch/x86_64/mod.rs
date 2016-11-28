use context;
use graphic;
use graphic::Display;
use memory;
use memory::AddressConverter;
use multiboot2;


pub fn init(argv: &[usize])
{
    // Copy memory information into the global context.
    let ref mut memory_region_manager = *context::GLOBAL_CONTEXT.memory_region_manager.lock();
    let multiboot_info = unsafe { multiboot2:: load(argv[0]) };
    if let Some(memory_map_tag) = multiboot_info.memory_map_tag() {
        for memory_area in memory_map_tag.memory_areas() {
            let mut memory_region = memory::region::Region::new();
            memory_region.set_base_addr(memory_area.base_addr as usize);
            memory_region.set_size(memory_area.length as usize);
            memory_region.set_state(memory::region::State::Free);

            memory_region_manager.append(memory_region);
        }
    }
}


pub fn obtain_kernel_console() -> Option<graphic::CharacterDisplay<'static>>
{
    lazy_static! {
        static ref TEXT_MODE_VRAM_ADDR: usize = 0xB8000.to_virtual_addr();
        static ref TEXT_MODE_WIDTH: usize     = 80;
        static ref TEXT_MODE_HEIGHT: usize    = 25;
    }

    let mut display = graphic::CharacterDisplay::new(*TEXT_MODE_VRAM_ADDR, graphic::Position(*TEXT_MODE_WIDTH, *TEXT_MODE_HEIGHT));
    display.clear_screen();
    Some(display)
}
