//! `Page` define a virtual memory region.
use crate::memory::address::VirtualAddress;

pub const SIZE: usize = 4096;

pub struct Page {
    number: usize,
}

impl Page {
    pub fn number(&self) -> usize {
        self.number
    }

    pub fn address(&self) -> VirtualAddress {
        self.number * SIZE
    }

    pub fn from_address(addr: VirtualAddress) -> Page {
        Page { number: addr / SIZE }
    }
}
