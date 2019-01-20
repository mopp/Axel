//! `Page` define a virtual memory region.
use crate::memory::address::VirtualAddress;

pub const SIZE: usize = 4096;

#[derive(Debug, Clone)]
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

    pub fn from_number(n: usize) -> Page {
        Page { number: n }
    }
}
