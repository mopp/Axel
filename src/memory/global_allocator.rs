use core::alloc::{GlobalAlloc, Layout};
use core::ptr;
use spin::Mutex;

pub struct GlobalAllocator(Mutex<Option<Allocator>>);

unsafe impl GlobalAlloc for GlobalAllocator {
    unsafe fn alloc(&self, _layout: Layout) -> *mut u8 {
        ptr::null_mut()
    }
    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {}
}

#[cfg(not(test))]
#[alloc_error_handler]
fn alloc_error(_: Layout) -> ! {
    unimplemented!("");
}

impl GlobalAllocator {
    pub const fn new() -> GlobalAllocator {
        GlobalAllocator(Mutex::new(None))
    }
}

struct Allocator {}

impl Allocator {}
