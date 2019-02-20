use bitfield::bitfield;
use lazy_static::lazy_static;
use spin::Mutex;
use x86_64::instructions::port::Port;
use super::pic;

lazy_static! {
    static ref CHANNEL0: Mutex<Port<u8>> = Mutex::new(Port::new(0x40));
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

    // Binary mode.
    // Channel 0.
    // Rate generator mode.
    // Ho/hi bytes.
    let mut r = ModeCommandRegister(0);
    r.set_operating_mode(0b010);
    r.set_access_mode(0b11);

    // Target timer tick frequency is 100Hz.
    let (h, l) = make_asymptotic_counter_value(100);

    unsafe {
        MODE_COMMAND_REGISTER.lock().write(r.0);

        CHANNEL0.lock().write(l);
        CHANNEL0.lock().write(h);

        pic::enable_timer_interrupt();

        // Enable interrupt.
        asm!("sti");
    };
}

fn make_asymptotic_counter_value(timer_frequency_hz: u32) -> (u8, u8) {
    // PIT clock is 1193181666... Hz
    // It is equals to 3579545 / 3.
    // Counter value is 16 bit value.
    // The range is from 1 to 65536 because dividing zero is forbidden and setting value 0 refers to 65536.
    // Minimum frequency is 3579545 / 3 / 65536 = 18.2065074.
    // Maximum frequency is 3579545 / 3 / 1 = 1193181.66667.
    // NOTE: counter value must not be 1 with mode2.
    // Maximum frequency in mode 2 is 3579545 / 3 / 2 = 596590.833333
    debug_assert!(18 <= timer_frequency_hz);
    debug_assert!(timer_frequency_hz < 1193181);

    let c = (3579545u32 / 3u32 / timer_frequency_hz) as u16;
    (((c >> 8) as u8), (c & 0xFF) as u8)
}
