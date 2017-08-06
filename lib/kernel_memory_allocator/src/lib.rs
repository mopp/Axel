#![feature(alloc)]
#![feature(allocator_api)]
#![feature(global_allocator)]
#![feature(heap_api)]
#![no_std]

extern crate alloc;
pub use alloc::allocator::{Alloc, Layout, AllocErr};

pub struct KernelMemoryAllocator;

unsafe impl<'a> Alloc for &'a KernelMemoryAllocator {
    unsafe fn alloc(&mut self, _: Layout) -> Result<*mut u8, AllocErr> {
        panic!("try to alloc: not yet implemented !");
    }

    unsafe fn dealloc(&mut self, _: *mut u8, _: Layout) {
        panic!("try to dealloc: not yet implemented !");
    }
}
