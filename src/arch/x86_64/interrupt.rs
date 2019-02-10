/// Interrupt descriptor table module.
use crate::memory::address::*;

mod descriptor;
mod handler;
mod table;
use descriptor::Descriptor;
use handler::sample_handler;
use table::Table;

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
