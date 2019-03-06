/// The trait to make n-byte aligned address (where n is a power of 2).
/// However, this functions accept alignment 1.
/// This means the number will not be changed.
pub trait Alignment {
    fn align_up(self, alignment: Self) -> Self;
    fn align_down(self, alignment: Self) -> Self;
}

impl Alignment for usize {
    fn align_up(self, alignment: Self) -> Self {
        let mask = alignment - 1;
        (self + mask) & (!mask)
    }

    fn align_down(self, alignment: Self) -> Self {
        let mask = alignment - 1;
        self & (!mask)
    }
}

#[cfg(test)]
mod test {
    use super::Alignment;

    #[test]
    fn test_alignment() {
        assert_eq!(0x1000.align_up(0x1000), 0x1000);
        assert_eq!(0x1000.align_down(0x1000), 0x1000);
        assert_eq!(0x1123.align_up(0x1000), 0x2000);
        assert_eq!(0x1123.align_down(0x1000), 0x1000);

        assert_eq!(0b1001.align_down(2), 0b1000);
        assert_eq!(0b1001.align_up(2), 0b1010);

        assert_eq!(0b1001.align_up(1), 0b1001);
        assert_eq!(0b1001.align_down(1), 0b1001);
    }
}
