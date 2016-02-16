extern crate multiboot;
extern crate core;

mod frame;

use graphic;
use alist::AList;
use graphic::Display;
use core::fmt::Write;
use core::mem;
use core::slice;
use arch::x86_32::memory::buddy_system::BuddyManager;


extern {
    static LD_KERNEL_BEGIN: usize;
    static LD_KERNEL_END :usize;
    static LD_KERNEL_SIZE: usize;
    // static LD_KERNEL_BSS_BEGIN : usize;
    // static LD_KERNEL_BSS_END : usize;
}

// TODO: Change value after setting pagin.
#[allow(dead_code)]
const VIRTUAL_KERNEL_BASE_ADDR: usize = 0;

macro_rules! address_of_var {
    ($x: expr) => (address_of_ref!(&$x));
}

macro_rules! address_of_ref {
    ($x: expr) => (($x as *const _) as usize);
}

macro_rules! align_up {
    ($align: expr, $n: expr) => {
        {
            let n = $n;
            let mask = $align - 1;
            (n + mask) & (!mask)
        }
    };
}

macro_rules! align_down {
    ($align: expr, $n: expr) => {
        {
            let n = $n;
            let mask = $align - 1;
            n & (!mask)
        }
    };
}


pub fn init(mboot: &multiboot::Multiboot)
{
    const TEXT_MODE_VRAM_ADDR: usize = 0xB8000;
    const TEXT_MODE_WIDTH: usize     = 80;
    const TEXT_MODE_HEIGHT: usize    = 25;

    let mut display = graphic::CharacterDisplay::new(TEXT_MODE_VRAM_ADDR, graphic::Position(TEXT_MODE_WIDTH, TEXT_MODE_HEIGHT));
    display.clear_screen();

    // Kernel memory info.
    let kernel_begin = address_of_var!(LD_KERNEL_BEGIN);
    let kernel_end   = address_of_var!(LD_KERNEL_END);

    writeln!(display, "Kernel").unwrap();
    writeln!(display, "Begin : 0x{:08x}", kernel_begin).unwrap();
    writeln!(display, "End   : 0x{:08x}", kernel_end).unwrap();
    writeln!(display, "Size  : {:}KB", address_of_var!(LD_KERNEL_SIZE) / 1024).unwrap();

    // Find physical free memory region for early allocator.
    let largest_region        = mboot.memory_regions().unwrap().max_by_key( |region| region.length() ).unwrap();
    let mut free_region_begin = largest_region.base_address() as usize;
    let free_region_end       = align_down!(frame::SIZE, (largest_region.base_address() + largest_region.length()) as usize);

    if free_region_begin < kernel_end {
        free_region_begin = kernel_end;
    }

    // Prepare allocator.
    let mut eallocator          = EarlyAllocator::new(free_region_begin, free_region_end);

    // Calculate the required size to allocate enough memory regions.
    let struct_size   = mem::size_of::<Frame>() + mem::size_of::<AList<Frame>>();
    let capacity      = eallocator.capacity();
    let num_frames    = capacity / frame::SIZE;
    let required_size = num_frames * struct_size;

    let capacity      = capacity - required_size;
    let num_frames    = capacity / frame::SIZE;

    // Allocate structures for memory management.
    let frames: &mut [Frame]               = eallocator.alloc_slice_mut(num_frames);
    let frame_list: &mut [AList<Frame>]    = eallocator.alloc_slice_mut(num_frames);
    let frame_list_head: &mut AList<Frame> = eallocator.alloc_type_mut();

    // Initialize frames.
    frame_list_head.init(&mut frames[0]);
    for i in 0..(num_frames) {
        frame_list[i].init(&mut frames[i]);
        frame_list_head.push_back(&mut frame_list[i]);
    }

    // Align managed begin memory address for block management.
    let free_region_begin = align_up!(frame::SIZE, free_region_begin);

    // Destruct allocator.
    let EarlyAllocator { addr_begin: free_region_begin, addr_end: free_region_end } = eallocator;
    drop(eallocator);

    writeln!(display, "Memory manage info").unwrap();
    writeln!(display, "Begin: 0x{:08x}", free_region_begin).unwrap();
    writeln!(display, "End  : 0x{:08x}", free_region_end).unwrap();
    writeln!(display, "Size : {}KB", (free_region_end - free_region_begin) / 1024).unwrap();
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
