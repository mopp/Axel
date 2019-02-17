use bitfield::bitfield;
use lazy_static::lazy_static;
use spin::Mutex;
use x86_64::instructions::port::Port;

lazy_static! {
    static ref CHANNEL0: Port<u8> = Port::new(0x40);
    static ref MODE_COMMAND_REGISTER: Mutex<Port<u8>> = Mutex::new(Port::new(0x43));
}

bitfield! {
    #[repr(C)]
    pub struct ModeCommandRegister(u8);
    impl Debug;
    // 0 refers to four-digit BCD.
    // 1 refers to 16-bit binary mode.
    bcd_binary_mode, set_bcd_binary_mode: 1, 0;
    operating_mode, set_operating_mode: 3, 1;
    access_mode, set_access_mode: 5, 4;
    select_channel, set_select_channnel: 7, 6;
}

pub fn init() {
    println!("init pit");
    println!("0x{:x}", unsafe { CHANNEL0.read() });
}
