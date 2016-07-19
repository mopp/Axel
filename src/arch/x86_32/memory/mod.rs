extern crate multiboot;
extern crate core;

#[macro_use]
mod macros;

mod buddy_system;
mod frame;

use alist::Node;
use arch::x86_32::memory::buddy_system::BuddyManager;
use arch::x86_32::memory::frame::Frame;
use core::mem;
use core::slice;


extern {
    // static LD_KERNEL_BEGIN: usize;
    static LD_KERNEL_END :usize;
    // static LD_KERNEL_SIZE: usize;
    // static LD_KERNEL_BSS_BEGIN : usize;
    // static LD_KERNEL_BSS_END : usize;
}

// TODO: Change value after setting pagin.
#[allow(dead_code)]
const VIRTUAL_KERNEL_BASE_ADDR: usize = 0;


pub fn init<'a, F>(mboot: &'a multiboot::Multiboot<'a, F>) where F: Fn(multiboot::PAddr, usize) -> Option<&'a [u8]>
{
    // Kernel memory info.
    // let kernel_begin = addr_of_var!(LD_KERNEL_BEGIN);
    let kernel_end   = addr_of_var!(LD_KERNEL_END);

    // Find physical free memory region for early allocator.
    let largest_region        = mboot.memory_regions().unwrap().max_by_key( |region| region.length() ).unwrap();
    let mut free_region_begin = largest_region.base_address() as usize;
    let free_region_end       = align_down!(frame::SIZE, (largest_region.base_address() + largest_region.length()) as usize);

    if free_region_begin < kernel_end {
        free_region_begin = kernel_end;
    }

    // Prepare allocator.
    let mut eallocator          = EarlyAllocator::new(free_region_begin, free_region_end);
    let bman: &mut BuddyManager = eallocator.alloc_type_mut();
    bman.num_each_free_frames   = eallocator.alloc_slice_mut(buddy_system::MAX_ORDER);
    bman.frame_lists            = eallocator.alloc_slice_mut(buddy_system::MAX_ORDER);

    // Calculate the required size to allocate enough memory regions.
    let struct_size   = mem::size_of::<Node<Frame>>();
    let capacity      = eallocator.capacity();
    let num_frames    = capacity / frame::SIZE;
    let required_size = num_frames * struct_size;

    let capacity      = capacity - required_size;
    let num_frames    = capacity / frame::SIZE;

    // Allocate structures for memory management.
    let frames: &mut [Node<Frame>] = eallocator.alloc_slice_mut(num_frames);

    // Destruct allocator.
    let free_region_begin = eallocator.addr_begin;
    drop(eallocator);

    // Align managed begin memory address for block management.
    let free_region_begin = align_up!(frame::SIZE, free_region_begin);

    // Initalize buddy manager.
    bman.frames           = frames;
    bman.base_addr        = free_region_begin;
    bman.num_total_frames = bman.frames.len();
    bman.init();

    // work well ?
    let tmp1 = bman.alloc(10);
    let tmp2 = bman.alloc(5);
    bman.free(tmp1.unwrap());
    bman.free(tmp2.unwrap());

    let tmp = bman.alloc(0).unwrap();
    let addr = bman.frame_addr(tmp);
    let array = unsafe {
        slice::from_raw_parts_mut(addr as *mut usize, tmp.get().size() / core::mem::size_of::<usize>())
    };
    for a in array {
        *a = *a + 100;
    }
}


#[allow(dead_code)]
fn to_physical(addr: &mut usize) -> usize
{
    *addr -= VIRTUAL_KERNEL_BASE_ADDR;
    *addr
}


#[allow(dead_code)]
fn to_virtual(addr: &mut usize) -> usize
{
    *addr += VIRTUAL_KERNEL_BASE_ADDR;
    *addr
}



struct EarlyAllocator {
    addr_begin: usize,
    addr_end: usize,
}


impl EarlyAllocator {
    fn new(free_region_begin: usize, free_region_end: usize) -> EarlyAllocator
    {
        EarlyAllocator {
            addr_begin: free_region_begin,
            addr_end: free_region_end,
        }
    }


    fn capacity(&self) -> usize
    {
        self.addr_end - self.addr_begin
    }


    fn align_addr_begin(&mut self, align: usize)
    {
        self.addr_begin = align_up!(align, self.addr_begin);
    }


    fn alloc_region(&mut self, size: usize, align: usize) -> *mut u8
    {
        self.align_addr_begin(align);
        let addr = self.addr_begin;
        self.addr_begin += size;

        addr as *mut u8
    }


    fn alloc_type_mut<'a, T: Sized>(&mut self) -> &'a mut T
    {
        let size  = mem::size_of::<T>();
        let align = mem::align_of::<T>();
        let ptr   = self.alloc_region(size, align);

        unsafe {
            mem::transmute(ptr)
        }
    }


    fn alloc_slice_mut<'a, T: Sized>(&mut self, num: usize) -> &'a mut [T]
    {
        let size  = mem::size_of::<T>();
        let align = mem::align_of::<T>();
        let ptr   = self.alloc_region(size * num, align);

        unsafe {
            slice::from_raw_parts_mut(ptr as *mut T, num)
        }
    }
}
