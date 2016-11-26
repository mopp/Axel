use core::mem;
use core::slice;
use super::Alignment;


struct EarlyAllocator {
    addr_begin: usize,
    addr_end: usize,
}


impl EarlyAllocator {
    fn new(addr_begin: usize, addr_end: usize) -> EarlyAllocator
    {
        EarlyAllocator {
            addr_begin: addr_begin,
            addr_end: addr_end,
        }
    }


    fn capacity(&self) -> usize
    {
        self.addr_end - self.addr_begin
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


    fn alloc_type_mut<'a, T: Sized>(&mut self) -> &'a mut T
    {
        let size  = mem::size_of::<T>();
        let align = mem::align_of::<T>();
        let ptr   = self.alloc(size, align);

        unsafe {
            mem::transmute(ptr)
        }
    }


    fn alloc_slice_mut<'a, T: Sized>(&mut self, num: usize) -> &'a mut [T]
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
}
