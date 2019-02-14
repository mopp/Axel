use bitfield::bitfield;
use lazy_static::lazy_static;
use x86_64::instructions::port::Port;

lazy_static! {
    static ref CHANNEL0: Port<u8> = Port::new(0x40);
    static ref MODE_COMMAND_REGISTER: Port<u8> = Port::new(0x43);
}

bitfield! {
    #[repr(C)]
    pub struct ModeRegister(u8);
    impl Debug;
    // 0 refers to four-digit BCD.
    // 1 refers to 16-bit binary mode.
    bcd_binary_mode, _: 1, 0;
    operating_mode, _: 3, 1;
    access_mode, _: 2, 4;
    select_channel, _: 2, 6;
}

pub fn init() {
    println!("init pit");
    println!("0x{:x}", unsafe { CHANNEL0.read() });
}
