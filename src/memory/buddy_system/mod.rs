pub mod buddy_manager;
pub mod buddy_frame;
mod list;

use self::buddy_frame::BuddyFrame;
pub use self::buddy_manager::BuddyManager;
use self::list::Node;
use super::EarlyAllocator;
use super::address::*;
use super::frame;
use core::mem;


pub fn allocate_buddy_manager<'b>(eallocator: &mut EarlyAllocator) -> BuddyManager {
    // Calculate the required size for frame information.
    let struct_size = mem::size_of::<Node<BuddyFrame>>();
    let capacity = eallocator.capacity();
    let num_frames = capacity / frame::SIZE;
    let required_size = num_frames * struct_size;

    let capacity = capacity - required_size;
    let num_frames = capacity / frame::SIZE;

    let frames = eallocator.alloc_slice_mut(num_frames);
    let base_addr = eallocator
        .available_space()
        .0
        .align_up(frame::SIZE)
        .to_physical_addr();

    BuddyManager::new(&mut frames[0] as *mut _, num_frames, base_addr, frame::SIZE)
}
