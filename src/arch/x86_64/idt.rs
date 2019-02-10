/// Interrupt descriptor table module.
use crate::memory::address::*;
use bitfield::bitfield;
use static_assertions::assert_eq_size;

pub type Handler = extern "x86-interrupt" fn(error: ErrorCode);
assert_eq_size!(usize, Handler);

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

bitfield! {
    #[repr(C)]
    pub struct ErrorCode(u32);
    impl Debug;
    external_event, _: 0;
    descriptor_location, _: 1;
    ti, _: 3, 2;
    segment_selector_index, _: 3, 15;
    reserved, _: 16, 31;
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

#[repr(C, packed)]
struct InterruptDescriptorTable {
    limit: u16,
    base: usize,
}
assert_eq_size!([u8; 10], InterruptDescriptorTable);

impl InterruptDescriptorTable {
    fn load(&self) {
        unsafe {
            asm!(".intel_syntax noprefix
                  lidt [$0]
                  "
                 :
                 : "r"(self)
                 :
                 : "intel");
        }
    }
}

#[repr(C)]
pub struct Table {
    descriptors: [Descriptor; 256],
}
assert_eq_size!([u8; 16 * 256], Table);

impl Table {
    #[inline(always)]
    fn clear_all(&mut self) {
        let addr = (self as *mut _) as usize;
        let size = 16 * 256;
        unsafe {
            use rlibc;
            rlibc::memset(addr as *mut u8, size, 0x00);
        }
    }

    fn load(&self) {
        let idtr = InterruptDescriptorTable {
            limit: core::mem::size_of::<Self>() as u16 - 1,
            base: (self as *const _) as usize,
        };

        idtr.load();
    }
}

// 0x00007E00 - 0x0007FFFF is free region.
// See x86 memory map.
// TODO: Put out this table address.
pub const TABLE_ADDRESS: PhysicalAddress = 0x6F000;

pub fn init() {
    println!("Initialize IDT");
    let table: &mut Table = unsafe { core::mem::transmute(TABLE_ADDRESS.to_virtual_addr()) };
    table.clear_all();

    for i in table.descriptors.iter_mut() {
        *i = Descriptor::new(sample_handler);
    }

    table.load();

    unsafe {
        asm!("int 0xFA" : : : : "intel");
    }
    println!("Return from interrupt");
}

extern "x86-interrupt" fn sample_handler(_: ErrorCode) {
    println!("Got interrupt");
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
