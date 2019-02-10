use super::descriptor::Descriptor;
use static_assertions::assert_eq_size;

#[repr(C, packed)]
struct InterruptDescriptorTableRegister {
    limit: u16,
    base: usize,
}
assert_eq_size!([u8; 10], InterruptDescriptorTableRegister);

impl InterruptDescriptorTableRegister {
    #[inline(always)]
    fn load(&self) {
        unsafe {
            asm!(".intel_syntax noprefix
                  lidt [$0]"
                  :
                  : "r"(self)
                  :
                  : "intel");
        }
    }
}

#[repr(C)]
pub struct InterruptDescriptorTable {
    pub descriptors: [Descriptor; 256],
}
assert_eq_size!([u8; 16 * 256], InterruptDescriptorTable);

impl InterruptDescriptorTable {
    #[inline(always)]
    pub fn clear_all(&mut self) {
        let addr = (self as *mut _) as usize;
        let size = 16 * 256;
        unsafe {
            use rlibc;
            rlibc::memset(addr as *mut u8, size, 0x00);
        }
    }

    #[inline(always)]
    pub fn load(&self) {
        let idtr = InterruptDescriptorTableRegister {
            limit: core::mem::size_of::<Self>() as u16 - 1,
            base: (self as *const _) as usize,
        };

        idtr.load();
    }
}
