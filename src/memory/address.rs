//! Module for functions/traits of address utils.

use spin::Mutex;


pub type PhysicalAddress = usize;
pub type VirtualAddress = usize;


// These variable are defined in the linker script and their value do NOT have any meanings.
// Their addressees have meanings.
extern {
    static KERNEL_ADDR_PHYSICAL_BEGIN: usize;
    static KERNEL_ADDR_PHYSICAL_END: usize;
    static KERNEL_ADDR_VIRTUAL_BEGIN: usize;
    static KERNEL_ADDR_BSS_BEGIN: usize;
    static KERNEL_SIZE_BSS: usize;
}


lazy_static! {
    static ref CURRENT_KERNEL_ADDR_PHYSICAL_END: Mutex<PhysicalAddress>   = Mutex::new(address_of(unsafe { &KERNEL_ADDR_PHYSICAL_END }));
}


#[inline(always)]
pub fn kernel_addr_begin_virtual() -> VirtualAddress
{
    address_of(unsafe { &KERNEL_ADDR_VIRTUAL_BEGIN })
}


#[inline(always)]
pub fn kernel_addr_begin_physical() -> PhysicalAddress
{
    address_of(unsafe { &KERNEL_ADDR_PHYSICAL_BEGIN })
}


#[inline(always)]
pub fn kernel_addr_end_physical() -> PhysicalAddress
{
    *(*CURRENT_KERNEL_ADDR_PHYSICAL_END).lock()
}


#[inline(always)]
pub fn update_kernel_addr_end_physical(new_addr: PhysicalAddress)
{
    *(*CURRENT_KERNEL_ADDR_PHYSICAL_END).lock() = new_addr
}


#[inline(always)]
pub fn kernel_bss_section_addr_begin() -> VirtualAddress
{
    address_of(unsafe { &KERNEL_ADDR_BSS_BEGIN })
}


#[inline(always)]
pub fn kernel_bss_section_size() -> VirtualAddress
{
    address_of(unsafe { &KERNEL_SIZE_BSS })
}


#[inline(always)]
pub fn address_of<T>(obj: &T) -> VirtualAddress
{
    (obj as *const _) as usize
}


pub trait ToPhysicalAddr {
    fn to_physical_addr(self) -> PhysicalAddress;
}


pub trait ToVirtualAddr {
    fn to_virtual_addr(self) -> VirtualAddress;
}


impl ToPhysicalAddr for VirtualAddress {
    fn to_physical_addr(self) -> usize
    {
        self - kernel_addr_begin_virtual()
    }
}


impl ToVirtualAddr for PhysicalAddress {
    fn to_virtual_addr(self) -> usize
    {
        self + kernel_addr_begin_virtual()
    }
}


/// The trait to make n-byte aligned address (where n is a power of 2).
/// However, this functions accept alignment 1.
/// This means the number will not be changed.
pub trait Alignment {
    fn align_up(self, alignment: Self) -> Self;
    fn align_down(self, alignment: Self) -> Self;
}


impl Alignment for usize {
    fn align_up(self, alignment: Self) -> Self
    {
        let mask = alignment - 1;
        (self + mask) & (!mask)
    }


    fn align_down(self, alignment: Self) -> Self
    {
        let mask = alignment - 1;
        self & (!mask)
    }
}


#[cfg(test)]
mod test {
    use super::{Alignment};


    #[test]
    fn test_alignment()
    {
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
