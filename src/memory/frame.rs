//! `Frame` define a physical memory region.
//! The size is 4096 and it corresponds page size.
pub const SIZE: usize = 4096;

use super::VirtualAddress;

pub struct Frame {
    number: usize,
}

impl Frame {
    pub fn from_addr(addr: VirtualAddress) -> Frame {
        Frame {
            number: addr / SIZE,
        }
    }

    pub fn level4_index(&self) -> usize {
        (self.number >> 27) & 0o777
    }

    pub fn level3_index(&self) -> usize {
        (self.number >> 18) & 0o777
    }

    pub fn level2_index(&self) -> usize {
        (self.number >> 9) & 0o777
    }

    pub fn level1_index(&self) -> usize {
        (self.number >> 0) & 0o777
    }
}


#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_index() {
        let f = Frame::from_addr(0x00000);

        assert_eq!(f.level4_index(), 0);
        assert_eq!(f.level3_index(), 0);
        assert_eq!(f.level2_index(), 0);
        assert_eq!(f.level1_index(), 0);

        let f = Frame::from_addr(0o456_555_456_123_0000);
        assert_eq!(f.level4_index(), 0o456);
        assert_eq!(f.level3_index(), 0o555);
        assert_eq!(f.level2_index(), 0o456);
        assert_eq!(f.level1_index(), 0o123);
    }
}
