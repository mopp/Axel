pub mod address;
mod buddy_system;
mod early_allocator;
mod frame;
mod frame_allocator;
mod paging;
pub mod region;

use context;
use core::mem;
use core::ptr::Unique;
use self::address::*;
use self::buddy_system::BuddyAllocator;
use self::early_allocator::EarlyAllocator;
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

#[inline]
fn allocate_buddy_manager<'b>(eallocator: &mut EarlyAllocator) -> BuddyAllocator<Frame> {
    // Calculate the required size for frame.
    let struct_size = mem::size_of::<Frame>();
    let capacity = eallocator.capacity();
    let count_frames = capacity / frame::SIZE;
    let required_size = count_frames * struct_size;

    let capacity = capacity - required_size;
    let count_frames = capacity / frame::SIZE;

    let frames = eallocator.alloc_slice_mut(count_frames);
    let base_addr = eallocator.available_space()
        .0
        .align_up(frame::SIZE)
        .to_physical_addr();

    let ptr = unsafe { Unique::new_unchecked(&mut frames[0] as *mut _) };
    BuddyAllocator::new(ptr, count_frames)
}

pub fn init() {
    let ref mut memory_region_manager = *context::GLOBAL_CONTEXT.memory_region_manager.lock();
    let kernel_addr_begin_physical = kernel_addr_begin_physical();
    let mut usable_memory_regions = memory_region_manager.regions_iter_with(region::State::Free).filter(|region| kernel_addr_begin_physical <= region.base_addr());

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
    let bman = allocate_buddy_manager(&mut eallocator);
    println!("Available memory: {} objects", bman.count_free_objs());

    paging::init(bman);
}
