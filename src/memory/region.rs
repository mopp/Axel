//! `Region` defines a physical memory region.
use super::PhysicalAddress;
use multiboot2::{MemoryArea, MemoryAreaIter, MemoryMapTag};

#[derive(Copy, Clone, PartialEq, Debug)]
pub enum State {
    Free,
    Used,
    Reserved,
}

#[derive(Clone, Debug)]
pub struct Region {
    base_addr: PhysicalAddress,
    size: usize,
    state: State,
}

impl Region {
    pub fn new(base_addr: PhysicalAddress, size: usize, state: State) -> Region {
        Region { base_addr: base_addr, size: size, state: state }
    }

    pub fn base_addr(&self) -> PhysicalAddress {
        self.base_addr
    }

    pub fn size(&self) -> usize {
        self.size
    }

    pub fn state(&self) -> State {
        self.state
    }
}

pub trait Adapter {
    type Item: Into<Region>;
    type Target: Iterator<Item = Self::Item>;

    fn iter(&self) -> AdapterIter<Self::Item, Self::Target>;
}

#[derive(Clone, Debug)]
pub struct AdapterIter<T: Into<Region>, U: Iterator<Item = T>> {
    iter: U,
}

impl<T: Into<Region>, U: Iterator<Item = T>> Iterator for AdapterIter<T, U> {
    type Item = Region;

    fn next(&mut self) -> Option<Region> {
        if let Some(area) = self.iter.next() {
            Some(area.into())
        } else {
            None
        }
    }
}

#[derive(Debug)]
pub struct Multiboot2Adapter {
    memory_map_tag: &'static MemoryMapTag,
}

impl Multiboot2Adapter {
    pub fn new(memory_map_tag: &'static MemoryMapTag) -> Multiboot2Adapter {
        Multiboot2Adapter { memory_map_tag }
    }
}

impl Adapter for Multiboot2Adapter {
    type Target = MemoryAreaIter;
    type Item = &'static MemoryArea;

    fn iter(&self) -> AdapterIter<&'static MemoryArea, MemoryAreaIter> {
        AdapterIter { iter: self.memory_map_tag.memory_areas() }
    }
}

impl Into<Region> for &'static MemoryArea {
    fn into(self) -> Region {
        Region::new(PhysicalAddress::new(self.start_address() as usize), self.size() as usize, State::Free)
    }
}
