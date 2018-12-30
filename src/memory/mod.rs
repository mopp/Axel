pub mod address;
mod buddy_system;
mod early_allocator;
mod frame;
mod paging;
pub mod region;

use self::address::*;
use self::buddy_system::BuddyAllocator;
use self::early_allocator::EarlyAllocator;
use core::mem;
use memory::frame::{Frame, FrameAdapter};
use memory::region::Region;

#[derive(Fail, Debug)]
pub enum Error {
    #[fail(display = "No usable memory region")]
    NoUsableMemory,
}

#[inline(always)]
pub fn clean_bss_section() {
    let begin = kernel_bss_section_addr_begin() as *mut u8;
    let size = kernel_bss_section_size() as i32;

    unsafe {
        use rlibc;
        rlibc::memset(begin, size, 0x00);
    }
}

#[inline(always)]
pub fn init<U: Into<Region>, T: Iterator<Item = U>>(regions: &region::Adapter<Item = U, Target = T>) -> Result<(), Error> {
    let kernel_addr_begin_physical = kernel_addr_begin_physical();

    let mut usable_memory_regions = regions.iter().filter(|region| kernel_addr_begin_physical <= region.base_addr());

    // TODO: Support multiple region.
    let free_memory_region = usable_memory_regions.nth(0).ok_or(Error::NoUsableMemory)?;

    // Use free memory region at the kernel tail.
    let kernel_addr_end_physical = kernel_addr_end_physical();
    let free_memory_region = free_memory_region;
    let free_region_addr_begin = kernel_addr_end_physical.to_virtual_addr();
    let free_region_addr_end = (free_memory_region.base_addr() + free_memory_region.size()).to_virtual_addr();
    let mut eallocator = EarlyAllocator::new(free_region_addr_begin, free_region_addr_end);

    // Calculate the required size for frame.
    let struct_size = mem::size_of::<Frame>();
    let capacity = eallocator.capacity();
    let count_frames = capacity / frame::SIZE;
    let required_size = count_frames * struct_size;

    let capacity = capacity - required_size;
    let count_frames = capacity / frame::SIZE;

    let frames = eallocator.allocate(count_frames);

    let mut bman = BuddyAllocator::new(frames, count_frames, FrameAdapter::new());
    println!("Available memory: {} objects", bman.count_free_objs());
    println!("Memory region size: {} KB", free_memory_region.size() / 1024);

    if let Some(obj) = bman.allocate(0) {
        println!("Allocate {:?}", obj);
        bman.free(obj);
    }

    paging::init(bman)
}
