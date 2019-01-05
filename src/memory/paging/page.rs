//! `Page` define a virtual memory region.
use super::PageIndex;
use crate::memory::address::VirtualAddress;

pub const SIZE: usize = 4096;

pub struct Page {
    number: usize,
}

impl Page {
    pub fn address(&self) -> VirtualAddress {
        self.number * SIZE
    }
}
