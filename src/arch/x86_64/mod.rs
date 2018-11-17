use super::Initialize;
use graphic;
use graphic::Display;
use memory::address::ToVirtualAddr;
use memory::address::VirtualAddress;
use memory::{self, region::Multiboot2Adapter};

pub struct Initializer;

impl Initialize for Initializer {
    fn init(argv: &[VirtualAddress]) -> Result<(), &'static str> {
        let multiboot_info = unsafe { multiboot2::load(argv[0]) };
        if let Some(memory_map_tag) = multiboot_info.memory_map_tag() {
            memory::init(&Multiboot2Adapter::new(memory_map_tag) as _)
        } else {
            Err("No memory map")
        }
    }

    fn obtain_kernel_console() -> Option<graphic::CharacterDisplay<'static>> {
        lazy_static! {
            static ref TEXT_MODE_VRAM_ADDR: usize = 0xB8000.to_virtual_addr();
            static ref TEXT_MODE_WIDTH: usize = 80;
            static ref TEXT_MODE_HEIGHT: usize = 25;
        }

        let mut display = graphic::CharacterDisplay::new(*TEXT_MODE_VRAM_ADDR, graphic::Position(*TEXT_MODE_WIDTH, *TEXT_MODE_HEIGHT));
        display.clear_screen();
        Some(display)
    }
}
