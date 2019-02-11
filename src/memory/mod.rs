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
use self::paging::table::Error as PageTableError;
pub use self::paging::IdenticalReMapRequest;
use self::region::Region;
use core::mem;
use core::ptr::Unique;
use failure::Fail;

#[derive(Fail, Debug)]
pub enum Error {
    #[fail(display = "No usable memory region")]
    NoUsableMemory,
    #[fail(display = "Page table error: {}", inner)]
    PageTable { inner: PageTableError },
}

impl From<PageTableError> for Error {
    fn from(inner: PageTableError) -> Error {
        Error::PageTable { inner }
    }
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
pub fn init<U: Into<Region>, T: Iterator<Item = U>>(regions: &region::Adapter<Item = U, Target = T>, remap_requests: &[IdenticalReMapRequest]) -> Result<(), Error> {
    let kernel_addr_begin_physical = kernel_addr_begin_physical();

    let mut usable_memory_regions = regions.iter().filter(|region| kernel_addr_begin_physical <= region.base_addr());

    // TODO: Support multiple region.
    let free_memory_region = usable_memory_regions.nth(0).ok_or(Error::NoUsableMemory)?;

    // Use free memory region at the kernel tail.
    let free_region_addr_begin = kernel_addr_end_virtual();
    let free_region_addr_end = (free_memory_region.base_addr() + free_memory_region.size()).to_virtual_addr();
    let mut allocator = EarlyAllocator::new(free_region_addr_begin, free_region_addr_end);

    let (frames, count_frames) = allocate_frames(&mut allocator);

    // Add the size of `frames` to the kernel size.
    let base_addr = allocator.to_addr_begin().to_physical_addr();
    update_kernel_addr_end_physical(base_addr);

    println!("free region: 0x{:x} - 0x{:x}, {}KB", base_addr, base_addr + count_frames * frame::SIZE, count_frames * frame::SIZE / 1024);

    // Initialize frames.
    unsafe {
        let base = base_addr / frame::SIZE;
        for (i, f) in core::slice::from_raw_parts_mut(frames.as_ptr(), count_frames).into_iter().enumerate() {
            f.set_number(base + i)
        }
    };

    let bman = BuddyAllocator::new(base_addr, frames, count_frames, FrameAdapter::new());
    println!("{} buddy objects", bman.count_free_objs());

    paging::init(remap_requests, bman)
}

#[inline(always)]
fn allocate_frames(allocator: &mut EarlyAllocator) -> (Unique<Frame>, usize) {
    allocator.align_addr_begin(frame::SIZE);

    let struct_size = mem::size_of::<Frame>();
    let capacity = allocator.capacity();
    let count_frames = capacity / frame::SIZE;
    let required_size = count_frames * struct_size;

    let capacity = capacity - required_size;
    let count_frames = capacity / frame::SIZE;

    let frames: Unique<Frame> = allocator.allocate(count_frames);

    allocator.align_addr_begin(frame::SIZE);
    debug_assert!(count_frames <= (allocator.capacity() / frame::SIZE));

    (frames, count_frames)
}
