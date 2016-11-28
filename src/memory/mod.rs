#[macro_export]
macro_rules! address_of {
    ($x: expr) => {{
        (&$x as *const _) as usize
    }};
}

mod frame;
mod early_allocator;
mod buddy_system;
pub mod region;

use context;
use self::early_allocator::EarlyAllocator;
use self::buddy_system::BuddyManager;
use alist::Node;
use self::frame::Frame;
use core::mem;



// These variable are defined in the linker script and their value do NOT have any meanings.
// Their addressees have meanings.
extern {
    static KERNEL_ADDR_PHYSICAL_BEGIN: usize;
    static KERNEL_ADDR_PHYSICAL_END: usize;
    static KERNEL_ADDR_VIRTUAL_BEGIN: usize;
    static KERNEL_ADDR_BSS_BEGIN: usize;
    static KERNEL_SIZE_BSS: usize;
}


/// The trait to make n-byte aligned address (where n is a power of 2).
/// However, this functions accept alignment 1.
/// This means the number will not be changed.
trait Alignment {
    fn align_up(self, alignment: Self) -> Self;
    fn align_down(self, alignment: Self) -> Self;
}


impl Alignment for usize {
    fn align_up(self, alignment: Self) -> Self
    {
        let mask = alignment - 1;
        (self + mask) & (!mask)
    }


    fn align_down(self, alignment: Self) -> Self
    {
        let mask = alignment - 1;
        self & (!mask)
    }
}


pub trait AddressConverter {
    fn to_physical_addr(self) -> usize;
    fn to_virtual_addr(self) -> usize;
}


impl AddressConverter for usize {
    fn to_physical_addr(self) -> usize
    {
        self - unsafe { address_of!(KERNEL_ADDR_VIRTUAL_BEGIN) }
    }


    fn to_virtual_addr(self) -> usize
    {
        self + unsafe { address_of!(KERNEL_ADDR_VIRTUAL_BEGIN) }
    }
}


pub fn clean_bss_section()
{
    unsafe {
        let begin = address_of!(KERNEL_ADDR_BSS_BEGIN) as *mut u8;
        let size  = address_of!(KERNEL_SIZE_BSS) as i32;

        use rlibc;
        rlibc::memset(begin, size, 0x00);
    }
}


pub fn init()
{
    let ref mut memory_region_manager = *context::GLOBAL_CONTEXT.memory_region_manager.lock();
    let kernel_addr_physical_begin = unsafe { address_of!(KERNEL_ADDR_PHYSICAL_BEGIN) };
    let mut usable_memory_regions = memory_region_manager.regions_iter_with(region::State::Free).filter(|region| kernel_addr_physical_begin <= region.base_addr());

    // TODO: Support multiple region.
    let free_memory_region = usable_memory_regions.nth(0);
    if free_memory_region.is_none() {
        unreachable!("No usable memory regions");
    }

    let kernel_addr_physical_end = unsafe { address_of!(KERNEL_ADDR_PHYSICAL_END) };
    let free_memory_region       = free_memory_region.unwrap();
    let free_region_addr_begin   = kernel_addr_physical_end.to_virtual_addr();
    let free_region_addr_end     = (free_memory_region.base_addr() + free_memory_region.size()).to_virtual_addr();

    let mut eallocator = EarlyAllocator::new(free_region_addr_begin, free_region_addr_end);
    let mut bman = allocate_buddy_manager(&mut eallocator);
}


fn allocate_buddy_manager<'b>(eallocator: &mut EarlyAllocator) -> &'b mut BuddyManager<'b>
{
    let bman: &mut BuddyManager = eallocator.alloc_type_mut();
    bman.num_each_free_frames   = eallocator.alloc_slice_mut(buddy_system::MAX_ORDER);
    bman.frame_lists            = eallocator.alloc_slice_mut(buddy_system::MAX_ORDER);

    // Calculate the required size for frame information.
    let struct_size   = mem::size_of::<Node<Frame>>();
    let capacity      = eallocator.capacity();
    let num_frames    = capacity / frame::SIZE;
    let required_size = num_frames * struct_size;

    let capacity      = capacity - required_size;
    let num_frames    = capacity / frame::SIZE;

    bman.frames           = eallocator.alloc_slice_mut(num_frames);
    bman.base_addr        = eallocator.available_space().0.align_up(frame::SIZE).to_physical_addr();
    bman.num_total_frames = bman.frames.len();
    bman.init();

    bman
}



#[cfg(test)]
mod test {
    use super::{Alignment};


    #[test]
    fn test_alignment()
    {
        assert_eq!(0x1000.align_up(0x1000), 0x1000);
        assert_eq!(0x1000.align_down(0x1000), 0x1000);
        assert_eq!(0x1123.align_up(0x1000), 0x2000);
        assert_eq!(0x1123.align_down(0x1000), 0x1000);

        assert_eq!(0b1001.align_down(2), 0b1000);
        assert_eq!(0b1001.align_up(2), 0b1010);

        assert_eq!(0b1001.align_up(1), 0b1001);
        assert_eq!(0b1001.align_down(1), 0b1001);
    }
}
