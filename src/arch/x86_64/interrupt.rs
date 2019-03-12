/// Interrupt descriptor table module.
use crate::memory::PhysicalAddress;

mod descriptor;
mod handler;
mod pic;
mod pit;
mod table;
pub use handler::InterruptFrame;
use table::InterruptDescriptorTable;

// 0x00007E00 - 0x0007FFFF is free region.
// See x86 memory map.
// TODO: Put out this table address.
pub const TABLE_ADDRESS: PhysicalAddress = PhysicalAddress::new(0x6E000);

pub fn init() {
    println!("Initialize IDT");
    let table: &mut InterruptDescriptorTable = unsafe { core::mem::transmute(TABLE_ADDRESS.to_virtual().into()) };
    table.init();
    table.load();

    // Runtime test.
    unsafe {
        asm!("int 0xFA" : : : : "intel");
    }
    println!("Return from interrupt");

    pic::init();
    pit::init();
}
