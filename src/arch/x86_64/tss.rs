use crate::memory::*;

#[derive(Debug)]
#[repr(C)]
pub struct TaskStateSegment {
    reserved0: u32,
    rsp0: usize,
    rsp1: usize,
    rsp2: usize,
    reserved1: usize,
    ist1: usize,
    ist2: usize,
    ist3: usize,
    ist4: usize,
    ist5: usize,
    ist6: usize,
    ist7: usize,
    reserved2: usize,
    reserved3: u16,
    io_map_base_address: u16,
}

impl TaskStateSegment {
    pub fn kernel_mode_stack(&self) -> VirtualAddress {
        VirtualAddress::new(self.rsp0)
    }

    pub fn set_kernel_mode_stack(&mut self, addr: usize) {
        self.rsp0 = addr;
    }
}
