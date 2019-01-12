pub mod address;
mod buddy_system;
mod early_allocator;
mod frame;
mod frame_allocator;
mod paging;
pub mod region;

use self::address::*;
use self::buddy_system::BuddyAllocator;
use self::early_allocator::EarlyAllocator;
pub use self::frame::{Frame, FrameAdapter};
pub use self::frame_allocator::FrameAllocator;
use self::region::Region;
use core::mem;
use core::ptr::Unique;

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

    let frames: Unique<Frame> = eallocator.allocate(count_frames);

    eallocator.align_addr_begin(frame::SIZE);
    let count_frames = capacity / frame::SIZE;
    let base_addr = eallocator.addr_begin.align_up(frame::SIZE).to_physical_addr();
    println!("managed free region: 0x{:x} - 0x{:x}, {}KB", base_addr, base_addr + count_frames * frame::SIZE, count_frames * frame::SIZE / 1024);

    unsafe {
        let base = base_addr / frame::SIZE;
        for (i, f) in core::slice::from_raw_parts_mut(frames.as_ptr(), count_frames).into_iter().enumerate() {
            f.set_number(base + i)
        }
    };

    let mut bman = BuddyAllocator::new(frames, count_frames, FrameAdapter::new());
    println!("{} buddy objects", bman.count_free_objs());

    if let Some(obj) = bman.allocate(0) {
        println!("Allocate {:?}", obj);
        bman.free(obj);
    }

    paging::init(bman)
}
