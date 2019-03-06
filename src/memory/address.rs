//! Module for functions/traits of address utils.
mod alignment;

pub use alignment::*;
use lazy_static::lazy_static;
use spin::Mutex;

pub type PhysicalAddress = usize;
pub type VirtualAddress = usize;

// These variable are defined in the linker script and their value do NOT have any meanings.
// Their addressees have meanings.
extern "C" {
    static KERNEL_ADDR_PHYSICAL_BEGIN: usize;
    static KERNEL_ADDR_PHYSICAL_END: usize;
    static KERNEL_ADDR_VIRTUAL_BEGIN: usize;
    static KERNEL_ADDR_BSS_BEGIN: usize;
    static KERNEL_ADDR_VIRTUAL_OFFSET: usize;
    static KERNEL_SIZE_BSS: usize;
}

lazy_static! {
    static ref CURRENT_KERNEL_ADDR_PHYSICAL_END: Mutex<PhysicalAddress> = Mutex::new(address_of(unsafe { &KERNEL_ADDR_PHYSICAL_END }));
}

#[inline(always)]
pub fn kernel_addr_begin_virtual() -> VirtualAddress {
    address_of(unsafe { &KERNEL_ADDR_VIRTUAL_BEGIN })
}

#[inline(always)]
pub fn kernel_addr_begin_physical() -> PhysicalAddress {
    address_of(unsafe { &KERNEL_ADDR_PHYSICAL_BEGIN })
}

#[inline(always)]
pub fn kernel_addr_end_physical() -> PhysicalAddress {
    *(*CURRENT_KERNEL_ADDR_PHYSICAL_END).lock()
}

#[inline(always)]
pub fn kernel_addr_end_virtual() -> VirtualAddress {
    kernel_addr_end_physical().to_virtual_addr()
}

#[inline(always)]
pub fn update_kernel_addr_end_physical(new_addr: PhysicalAddress) {
    *(*CURRENT_KERNEL_ADDR_PHYSICAL_END).lock() = new_addr
}

#[inline(always)]
pub fn kernel_bss_section_addr_begin() -> VirtualAddress {
    address_of(unsafe { &KERNEL_ADDR_BSS_BEGIN })
}

#[inline(always)]
pub fn kernel_bss_section_size() -> VirtualAddress {
    address_of(unsafe { &KERNEL_SIZE_BSS })
}

#[inline(always)]
pub fn address_of<T>(obj: &T) -> VirtualAddress {
    (obj as *const _) as usize
}

pub trait ToPhysicalAddr {
    fn to_physical_addr(self) -> PhysicalAddress;
}

pub trait ToVirtualAddr {
    fn to_virtual_addr(self) -> VirtualAddress;
}

impl ToPhysicalAddr for VirtualAddress {
    fn to_physical_addr(self) -> PhysicalAddress {
        self - address_of(unsafe { &KERNEL_ADDR_VIRTUAL_OFFSET })
    }
}

impl ToVirtualAddr for PhysicalAddress {
    fn to_virtual_addr(self) -> VirtualAddress {
        self + address_of(unsafe { &KERNEL_ADDR_VIRTUAL_OFFSET })
    }
}
