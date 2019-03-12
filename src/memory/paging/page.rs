//! `Page` define a virtual memory region.
use crate::memory::VirtualAddress;

pub const SIZE: usize = 4096;

#[derive(Debug, Clone)]
pub struct Page {
    number: usize,
}

impl Page {
    // pub fn number(&self) -> usize {
    //     self.number
    // }

    pub fn address(&self) -> VirtualAddress {
        VirtualAddress::new(self.number * SIZE)
    }

    pub fn from_address(addr: VirtualAddress) -> Page {
        Page { number: addr.into() / SIZE }
    }

    pub fn from_number(n: usize) -> Page {
        Page { number: n }
    }

    pub unsafe fn to_slice_mut<T>(&mut self) -> &mut [T] {
        let addr: usize = self.address().into();
        core::slice::from_raw_parts_mut(addr as *mut T, 4096 / core::mem::size_of::<T>())
    }
}
