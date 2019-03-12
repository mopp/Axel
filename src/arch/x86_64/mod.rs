use super::Error;
use super::Initialize;
use crate::graphic;
use crate::graphic::Display;
use crate::memory::{self, region::Multiboot2Adapter, IdenticalReMapRequest, PhysicalAddress};
use lazy_static::lazy_static;

mod interrupt;
mod thread;
mod tss;

pub use thread::Thread;

const VRAM_ADDR: PhysicalAddress = PhysicalAddress::new(0xB8000);

pub struct Initializer;

impl Initialize for Initializer {
    fn init(argv: &[usize]) -> Result<(), Error> {
        let rs = [
            // Kernel stack info (see boot.asm).
            IdenticalReMapRequest::new(PhysicalAddress::new(argv[1]), argv[2]),
            // FIXME: do not hard-code vram address.
            IdenticalReMapRequest::new(VRAM_ADDR, 1),
            // IDT
            IdenticalReMapRequest::new(interrupt::TABLE_ADDRESS, 1),
        ];

        let multiboot_info = unsafe { multiboot2::load(argv[0]) };
        let memory_map_tag = multiboot_info.memory_map_tag().ok_or(Error::NoMemoryMap)?;
        memory::init(&Multiboot2Adapter::new(memory_map_tag) as _, &rs).map_err(Into::<Error>::into)?;
        interrupt::init();

        Ok(())
    }

    fn obtain_kernel_console() -> Option<graphic::CharacterDisplay<'static>> {
        lazy_static! {
            static ref TEXT_MODE_VRAM_ADDR: usize = VRAM_ADDR.to_virtual().into();
            static ref TEXT_MODE_WIDTH: usize = 80;
            static ref TEXT_MODE_HEIGHT: usize = 25;
        }

        let mut display = graphic::CharacterDisplay::new(*TEXT_MODE_VRAM_ADDR, graphic::Position(*TEXT_MODE_WIDTH, *TEXT_MODE_HEIGHT));
        display.clear_screen();
        Some(display)
    }
}
