//! The `EarlyAllocator` is the simplest memory allocator.
//! It cutouts a memory region from the given memory region.

use super::address::Alignment;
use core::mem;
use core::ptr::Unique;

pub struct EarlyAllocator {
    pub addr_begin: usize,
    addr_end: usize,
}

impl EarlyAllocator {
    pub fn new(addr_begin: usize, addr_end: usize) -> EarlyAllocator {
        debug_assert!(addr_begin < addr_end);

        EarlyAllocator { addr_begin, addr_end }
    }

    pub fn capacity(&self) -> usize {
        self.addr_end - self.addr_begin
    }

    pub fn align_addr_begin(&mut self, alignment: usize) {
        self.addr_begin = self.addr_begin.align_up(alignment);
    }

    pub fn allocate<T>(&mut self, count: usize) -> Unique<T> {
        self.align_addr_begin(mem::align_of::<T>());
        let addr = self.addr_begin;
        let size = mem::size_of::<T>() * count;
        self.addr_begin += size;

        unsafe { Unique::new_unchecked(addr as *mut _) }
    }

    pub fn to_addr_begin(self) -> usize {
        self.addr_begin
    }
}

#[cfg(test)]
mod test {
    use super::EarlyAllocator;
    use std::mem;

    #[test]
    fn test_capacity() {
        let eallocator = EarlyAllocator::new(0x0000, 0x1000);

        assert_eq!(eallocator.capacity(), 0x1000);
    }

    struct Object {
        hoge: usize,
    }

    #[test]
    fn test_allocate() {
        let mut eallocator = EarlyAllocator::new(0x0000, 0x1000);
        let mut capacity = eallocator.capacity();

        let _ = eallocator.allocate::<Object>(10);
        capacity -= mem::size_of::<Object>() * 10;
        assert_eq!(eallocator.capacity(), capacity);

        let _ = eallocator.allocate::<Object>(50);
        capacity -= mem::size_of::<Object>() * 50;
        assert_eq!(eallocator.capacity(), capacity);
    }
}
