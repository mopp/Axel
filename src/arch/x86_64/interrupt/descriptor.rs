use super::handler::Handler;
use bitfield::bitfield;
use static_assertions::assert_eq_size;

bitfield! {
    #[repr(C)]
    pub struct Flags(u16);
    impl Debug;
    interrupt_stack_table, set_interrupt_stack_table: 2, 0;
    reserved1, _: 7, 2;
    descriptor_type, set_descriptor_type: 11, 8;
    reserved2, _: 13, 12;
    privilege_level, set_privilege_level: 14, 13;
    present, set_present: 15;
}

/// An entry for interrupt descriptor table.
#[derive(Debug)]
#[repr(C)]
pub struct Descriptor {
    offset_low: u16,
    segment_selector: u16,
    flags: Flags,
    offset_middle: u16,
    offset_high: u32,
    reserved: u32,
}
assert_eq_size!([u8; 16], Descriptor);

impl Descriptor {
    pub fn new(handler: Handler) -> Descriptor {
        let addr = (handler as *const Handler) as usize;
        let (offset_low, offset_middle, offset_high) = Descriptor::parse_addr(addr);

        Descriptor {
            offset_low,
            segment_selector: 1 << 3,
            flags: Flags(0b1_00_0_1110_000_0_0_000),
            offset_middle,
            offset_high,
            reserved: 0,
        }
    }

    #[inline(always)]
    fn parse_addr(addr: usize) -> (u16, u16, u32) {
        ((addr & 0xFFFF) as u16, ((addr >> 16) & 0xFFFF) as u16, ((addr >> 32) & 0xFFFFFFFF) as u32)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn flags_test() {
        assert_eq!(0b101, Flags(0b101).interrupt_stack_table());
        assert_eq!(0b1001, Flags(0b1001_000_00_000).descriptor_type());
        assert_eq!(0b11, Flags(0b11_0_0000_000_00_000).privilege_level());
        assert_eq!(true, Flags(0b1_00_0_0000_000_00_000).present());
    }

    #[test]
    fn descriptor_test() {
        assert_eq!((0x1111, 0x2222, 0x4444_3333), Descriptor::parse_addr(0x4444_3333_2222_1111))
    }
}
