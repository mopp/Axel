use crate::memory::address::VirtualAddress;

pub trait PageIndex {
    fn level4_index(self) -> usize;
    fn level3_index(self) -> usize;
    fn level2_index(self) -> usize;
    fn level1_index(self) -> usize;
    fn offset(self) -> usize;
}

impl PageIndex for VirtualAddress {
    fn level4_index(self) -> usize {
        (self.into() >> 39) & 0o777
    }

    fn level3_index(self) -> usize {
        (self.into() >> 30) & 0o777
    }

    fn level2_index(self) -> usize {
        (self.into() >> 21) & 0o777
    }

    fn level1_index(self) -> usize {
        (self.into() >> 12) & 0o777
    }

    fn offset(self) -> usize {
        self.into() & 0xFFF
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::memory::address::VirtualAddress;

    #[test]
    fn test_index_functions() {
        let addr: VirtualAddress = 0o123_456_712_345_1234;

        assert_eq!(addr.offset(), 0o1234);
        assert_eq!(addr.level1_index(), 0o345);
        assert_eq!(addr.level2_index(), 0o712);
        assert_eq!(addr.level3_index(), 0o456);
        assert_eq!(addr.level4_index(), 0o123);
    }
}
