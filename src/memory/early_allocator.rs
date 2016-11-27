//! The `EarlyAllocator` is the simplest memory allocator.
//! It cutouts a memory region from the given memory region.

use core::mem;
use core::slice;
use super::Alignment;


pub struct EarlyAllocator {
    addr_begin: usize,
    addr_end: usize,
}


impl EarlyAllocator {
    pub fn new(addr_begin: usize, addr_end: usize) -> EarlyAllocator
    {
        debug_assert!(addr_begin < addr_end);

        EarlyAllocator {
            addr_begin: addr_begin,
            addr_end: addr_end,
        }
    }


    pub fn capacity(&self) -> usize
    {
        self.addr_end - self.addr_begin
    }


    pub fn available_space(&self) -> (usize, usize)
    {
        (self.addr_begin, self.addr_end)
    }


    fn align_addr_begin(&mut self, alignment: usize)
    {
        self.addr_begin = self.addr_begin.align_up(alignment);
    }


    fn alloc(&mut self, size: usize, alignment: usize) -> *mut u8
    {
        self.align_addr_begin(alignment);
        let addr = self.addr_begin;
        self.addr_begin += size;

        addr as *mut u8
    }


    pub fn alloc_type_mut<'a, T: Sized>(&mut self) -> &'a mut T
    {
        let size  = mem::size_of::<T>();
        let align = mem::align_of::<T>();
        let ptr   = self.alloc(size, align);

        unsafe {
            mem::transmute(ptr)
        }
    }


    pub fn alloc_slice_mut<'a, T: Sized>(&mut self, num: usize) -> &'a mut [T]
    {
        let size  = mem::size_of::<T>();
        let align = mem::align_of::<T>();
        let ptr   = self.alloc(size * num, align);

        unsafe {
            slice::from_raw_parts_mut(ptr as *mut T, num)
        }
    }
}


#[cfg(test)]
mod test {
    use super::{EarlyAllocator};


    #[test]
    fn test_early_allocator_new()
    {
        let eallocator = EarlyAllocator::new(0x0000, 0x1000);

        assert_eq!(eallocator.capacity(), 0x1000);
    }


    #[test]
    fn test_early_allocator_alloc()
    {
        let mut eallocator = EarlyAllocator::new(0x0000, 0x1000);
        let mut capacity = eallocator.capacity();

        eallocator.alloc(0x100, 0x2);
        capacity -= 0x100;
        assert_eq!(eallocator.capacity(), capacity);

        eallocator.alloc(0x100, 0x2);
        capacity -= 0x100;
        assert_eq!(eallocator.capacity(), capacity);

        let number: &mut u64 = eallocator.alloc_type_mut();
        capacity -= 8;
        assert_eq!(eallocator.capacity(), capacity);

        let number: &mut [u64] = eallocator.alloc_slice_mut(9);
        capacity -= 8 * 9;
        assert_eq!(eallocator.capacity(), capacity);

        let mut eallocator2 = EarlyAllocator::new(0x1234, 0x2000);
        eallocator2.alloc(0x100, 0x100);
        assert_eq!(eallocator2.capacity(), 0x2000 - 0x1300 - 0x100);
    }
}
