//! `Region` defines a physical memory region.
use super::PhysicalAddress;
use core::mem;
use core::ptr;
use core::result::Result;

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

const MAX_REGION_COUNT: usize = 8;

pub struct RegionManager {
    regions: [Region; MAX_REGION_COUNT],
    count: usize,
}

impl RegionManager {
    pub fn new() -> RegionManager {
        let regions = unsafe {
            let mut regions: [Region; MAX_REGION_COUNT] = mem::uninitialized();

            for r in regions.iter_mut() {
                ptr::write(r, Region::new(0, 0, State::Reserved));
            }

            regions
        };

        RegionManager { regions: regions, count: 0 }
    }

    pub fn append(&mut self, r: Region) -> Result<(), &'static str> {
        if self.count == MAX_REGION_COUNT {
            return Err("capacity over");
        }

        self.regions[self.count] = r;
        self.count += 1;

        Ok(())
    }

    pub fn iter(&self) -> impl Iterator<Item = &Region> {
        self.regions[0..self.count].into_iter()
    }
}

#[cfg(test)]
mod test {
    use super::{Region, RegionManager, State};

    #[test]
    fn test_append() {
        let mut manager = RegionManager::new();

        for r in manager.iter() {
            panic!("manager has no regions here");
        }

        manager.append(Region::new(0, 0x1000, State::Free));

        for r in manager.iter() {
            assert_eq!(r.base_addr(), 0);
        }
    }
}
