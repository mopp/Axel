//! Module for functions/traits of address utils.
mod alignment;

pub use alignment::*;
use core::fmt;
use lazy_static::lazy_static;
use spin::Mutex;

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct PhysicalAddress(usize);

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VirtualAddress(usize);

impl PhysicalAddress {
    pub fn new(addr: usize) -> PhysicalAddress {
        PhysicalAddress(addr)
    }

    pub fn to_virtual(self) -> VirtualAddress {
        VirtualAddress(self.0 + kernel_addr_virtual_offset())
    }
}

impl VirtualAddress {
    pub fn new(addr: usize) -> VirtualAddress {
        VirtualAddress(addr)
    }

    pub fn to_physical(self) -> PhysicalAddress {
        PhysicalAddress(self.0 - kernel_addr_virtual_offset())
    }
}

impl fmt::LowerHex for PhysicalAddress {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:x}", self.0)
    }
}

impl fmt::LowerHex for VirtualAddress {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:x}", self.0)
    }
}

// https://users.rust-lang.org/t/how-to-use-self-or-any-variable-in-macro-generated-function-body/6264/2
// > any identifier used in an expression passed to a macro is assumed to come from outside of a macro
macro_rules! impl_both {
    ($target: ty, $sel: ident, $body: block) => {
        impl Into<$target> for VirtualAddress {
            fn into($sel) -> $target {
                $body
            }
        }

        impl Into<$target> for PhysicalAddress {
            fn into($sel) -> $target {
                return $body
            }
        }
    };
}

impl_both! { usize, self, {self.0 as usize} }
impl_both! { u64, self, {self.0 as u64} }
impl_both! { u8, self, {self.0 as u8} }
impl_both! { *mut u8, self, {self.0 as *mut u8} }

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
    static ref CURRENT_KERNEL_ADDR_PHYSICAL_END: Mutex<PhysicalAddress> = Mutex::new(physical_address_of(unsafe { &KERNEL_ADDR_PHYSICAL_END }));
}

#[inline(always)]
fn kernel_addr_virtual_offset() -> usize {
    address_of(unsafe { &KERNEL_ADDR_VIRTUAL_OFFSET })
}

#[inline(always)]
pub fn kernel_addr_begin_virtual() -> VirtualAddress {
    virtual_address_of(unsafe { &KERNEL_ADDR_VIRTUAL_BEGIN })
}

#[inline(always)]
pub fn kernel_addr_begin_physical() -> PhysicalAddress {
    physical_address_of(unsafe { &KERNEL_ADDR_PHYSICAL_BEGIN })
}

#[inline(always)]
pub fn kernel_addr_end_physical() -> PhysicalAddress {
    *(*CURRENT_KERNEL_ADDR_PHYSICAL_END).lock()
}

#[inline(always)]
pub fn kernel_addr_end_virtual() -> VirtualAddress {
    kernel_addr_end_physical().to_virtual()
}

#[inline(always)]
pub fn update_kernel_addr_end_physical(new_addr: PhysicalAddress) {
    *(*CURRENT_KERNEL_ADDR_PHYSICAL_END).lock() = new_addr
}

#[inline(always)]
pub fn kernel_bss_section_addr_begin() -> VirtualAddress {
    virtual_address_of(unsafe { &KERNEL_ADDR_BSS_BEGIN })
}

#[inline(always)]
pub fn kernel_bss_section_size() -> usize {
    address_of(unsafe { &KERNEL_SIZE_BSS })
}

#[inline(always)]
fn address_of<T>(obj: &T) -> usize {
    (obj as *const _) as usize
}

#[inline(always)]
pub fn virtual_address_of<T>(obj: &T) -> VirtualAddress {
    VirtualAddress((obj as *const _) as usize)
}

#[inline(always)]
pub fn physical_address_of<T>(obj: &T) -> PhysicalAddress {
    PhysicalAddress((obj as *const _) as usize)
}
