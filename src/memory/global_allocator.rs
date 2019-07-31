use super::{address::Alignment, BuddyAllocator, Frame, FrameAdapter, ACTIVE_PAGE_TABLE};
use core::alloc::{GlobalAlloc, Layout};
use intrusive_collections::LinkedList;
use spin::Mutex;

/// Wrapper struct for virtual memory allocator (`Allocator`).
/// The purpose is just to provide lock.
pub struct GlobalAllocator(Mutex<Option<HeapAllocator>>);

unsafe impl GlobalAlloc for GlobalAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        if let Some(ref mut allocator) = *self.0.lock() {
            if let Some(ptr) = allocator.alloc(layout) {
                ptr
            } else {
                panic!("No memory")
            }
        } else {
            panic!("GlobalAllocator is not initialized yet")
        }
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        if let Some(ref allocator) = *self.0.lock() {
            allocator.dealloc(ptr, layout)
        } else {
            panic!("GlobalAllocator is not initialized yet")
        }
    }
}

#[cfg(not(test))]
#[alloc_error_handler]
fn alloc_error(layout: Layout) -> ! {
    panic!("Allocation error: {:?}", layout)
}

impl GlobalAllocator {
    pub const fn new() -> GlobalAllocator {
        GlobalAllocator(Mutex::new(None))
    }

    pub fn init(&self, allocator: HeapAllocator) {
        *self.0.lock() = Some(allocator);
    }
}

pub struct HeapAllocator {
    // used_pages: LinkedList<FrameAdapter>,
    addr_begin: usize,
    addr_end: usize,
}

impl HeapAllocator {
    pub fn new(allocator: &mut BuddyAllocator<FrameAdapter, Frame>) -> Option<HeapAllocator> {
        let mut pages = LinkedList::new(FrameAdapter::new());

        // 2^5 * 4096 = 128KB.
        let frame = allocator.allocate(5)?;
        let (addr_begin, addr_end) = ACTIVE_PAGE_TABLE.lock().auto_continuous_map(&frame, allocator)?;

        pages.push_front(frame);

        // Some(HeapAllocator { used_pages: pages, addr_begin, addr_end })
        Some(HeapAllocator { addr_begin, addr_end })
    }
}

impl HeapAllocator {
    fn alloc(&mut self, layout: Layout) -> Option<*mut u8> {
        self.addr_begin.align_up(layout.align());

        if self.addr_end <= self.addr_begin + layout.size() {
            return None;
        }

        let addr = self.addr_begin;
        self.addr_begin += layout.size();

        Some(addr as *mut u8)
    }

    fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {}
}
