mod buddy_system;
mod early_allocator;
mod frame;
mod frame_allocator;
mod paging;
pub mod address;
pub mod region;

use context;
use self::address::*;
use self::early_allocator::EarlyAllocator;
use self::frame_allocator::FrameAllocator;
use self::frame::Frame;


#[inline(always)]
pub fn clean_bss_section() {
    let begin = kernel_bss_section_addr_begin() as *mut u8;
    let size = kernel_bss_section_size() as i32;

    unsafe {
        use rlibc;
        rlibc::memset(begin, size, 0x00);
    }
}


pub fn init() {
    let ref mut memory_region_manager = *context::GLOBAL_CONTEXT.memory_region_manager.lock();
    let kernel_addr_begin_physical = kernel_addr_begin_physical();
    let mut usable_memory_regions = memory_region_manager
        .regions_iter_with(region::State::Free)
        .filter(|region| kernel_addr_begin_physical <= region.base_addr());

    // TODO: Support multiple region.
    let free_memory_region = usable_memory_regions.nth(0);
    if free_memory_region.is_none() {
        unreachable!("No usable memory regions");
    }

    let kernel_addr_end_physical = kernel_addr_end_physical();
    let free_memory_region = free_memory_region.unwrap();
    let free_region_addr_begin = kernel_addr_end_physical.to_virtual_addr();
    let free_region_addr_end = (free_memory_region.base_addr() + free_memory_region.size()).to_virtual_addr();

    let mut eallocator = EarlyAllocator::new(free_region_addr_begin, free_region_addr_end);
    let mut bman = buddy_system::allocate_buddy_manager(&mut eallocator);
    println!("Available memory: {} KB", bman.free_memory_size() / 1024);

    paging::init(bman);
}
