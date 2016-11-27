#![allow(dead_code)]

#[derive(Copy, Clone, PartialEq, Debug)]
pub enum State {
    Free,
    Used,
    Reserved
}


impl Default for State {
    fn default() -> State { State::Free }
}


#[derive(Copy, Clone, Default)]
pub struct Region {
    base_addr: usize,
    size: usize,
    state: State,
}


impl Region {
    pub fn new() -> Region
    {
        Default::default()
    }


    pub fn set_base_addr(&mut self, base_addr: usize)
    {
        self.base_addr = base_addr;
    }


    pub fn base_addr(&self) -> usize
    {
        self.base_addr
    }


    pub fn set_size(&mut self, size: usize)
    {
        self.size = size;
    }


    pub fn size(&self) -> usize
    {
        self.size
    }


    pub fn set_state(&mut self, state: State)
    {
        self.state = state;
    }


    pub fn state(&self) -> State
    {
        self.state
    }


    pub fn is_valid(&self) -> bool
    {
        match self {
            _ if self.size == 0 => false,
            _  => true,
        }
    }
}


const MAX_REGION_COUNT: usize = 5;


pub struct RegionManager {
    regions: [Region; MAX_REGION_COUNT],
    valid_region_count: usize,
}


impl RegionManager {
    pub fn new() -> RegionManager
    {
        RegionManager {
            regions: [Default::default(); MAX_REGION_COUNT],
            valid_region_count: 0,
        }
    }


    pub fn append(&mut self, r: Region)
    {
        if self.valid_region_count == MAX_REGION_COUNT {
            panic!("Region manager does not have enough space.");
        }

        if r.is_valid() == false {
            panic!("Invalid region.");
        }

        self.regions[self.valid_region_count] = r;
        self.valid_region_count += 1;
    }


    pub fn regions(&self) -> &[Region]
    {
        &self.regions
    }


    pub fn regions_iter_with<'a>(&'a self, state: State) -> impl Iterator<Item = &'a Region> {
        self.regions().into_iter().filter(move |region| (region.state() == state) && (region.is_valid() == true))
    }
}


#[cfg(test)]
mod test {
    use super::{Region, RegionManager, State};


    #[test]
    fn test_region_create()
    {
        let mr: Region = Region::new();

        assert_eq!(mr.base_addr, 0);
        assert_eq!(mr.size, 0);
        assert_eq!(mr.state, State::Free);
    }


    #[test]
    fn test_region_is_valid()
    {
        let mut mr: Region = Region::new();

        assert_eq!(mr.is_valid(), false);

        mr.size = 0x100;
        assert_eq!(mr.is_valid(), true);
    }


    #[test]
    fn test_region_state()
    {
        let mut mr: Region = Region::new();
    }


    #[test]
    fn test_region_manager_create()
    {
        let r_man = RegionManager::new();
        assert_eq!(r_man.valid_region_count, 0);
    }


    #[test]
    fn test_region_manager_append()
    {
        let mut r_man = RegionManager::new();
        let mut mr: Region = Region::new();
        mr.size = 0x100;

        r_man.append(mr);

        assert_eq!(r_man.regions[0].size, mr.size);
    }


    #[test]
    fn test_region_manager_iter()
    {
        let mut r_man = RegionManager::new();
        let mut mr: Region = Region::new();
        mr.size = 0x100;

        for r in r_man.regions().iter() {
            assert_eq!(r.is_valid(), false);
        }

        for _ in r_man.regions_iter_with(State::Free) {
            unreachable!();
        }

        r_man.append(mr);

        for r in r_man.regions_iter_with(State::Free) {
            assert_eq!(r.size, mr.size);
        }

        for _ in r_man.regions_iter_with(State::Used) {
            unreachable!();
        }
    }
}
