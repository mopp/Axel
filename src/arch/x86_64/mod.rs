use graphic::Display;
use graphic;
use memory::address::VirtualAddress;
use memory::address::ToVirtualAddr;
use memory::region::RegionManager;
use memory::region::Region;
use memory::region::State;
use multiboot2;


pub fn init(argv: &[VirtualAddress], region_manager: &mut RegionManager) -> Result<(), &'static str> {
    // Copy memory information into the global context.
    let multiboot_info = unsafe { multiboot2:: load(argv[0]) };
    if let Some(memory_map_tag) = multiboot_info.memory_map_tag() {
        for area in memory_map_tag.memory_areas() {
            // let mut memory_region = memory::region::Region::new();
            // memory_region.set_base_addr(memory_area.start_address());
            // memory_region.set_size(memory_area.size());
            // memory_region.set_state(memory::region::State::Free);

            let r = Region::new(area.start_address(), area.size(), State::Free);
            match region_manager.append(r) {
                Ok(()) => {}
                Err(reason) => {
                    return Err(reason)
                }
            }
        }
        Ok(())
    } else {
        Err("no available memory")
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
