//! GPIO module.
//! http://www.valvers.com/open-software/raspberry-pi/step01-bare-metal-programming-in-cpt1/
//! https://www.raspberrypi.org/documentation//hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf
//!  LED ON, Zero -> CLR
//!        , Pi 1 B+ -> Set

use super::addr::Addr;
extern crate core;


#[allow(dead_code)]
pub enum Function {
    Input      = 0b000,
    Output     = 0b001,
    Alternate0 = 0b100,
    Alternate1 = 0b101,
    Alternate2 = 0b110,
    Alternate3 = 0b111,
    Alternate4 = 0b011,
    Alternate5 = 0b010,
}


pub enum Output {
    Clear,
    Set,
}


#[allow(dead_code)]
pub enum Pin {
    #[cfg(any(RPi_a, RPi_aplus))]
    OkLed = 16,
    #[cfg(any(RPi_zero, RPi_bplus, RPi2_b))]
    OkLed = 47,

    Spi0Ce1N = 7,
    Spi0Ce0N = 8,
    Spi0Miso = 9,
    Spi0Mosi = 10,
    Spi0Sclk = 11,

    // ILI 9340 (pitft 2.2) GPIO PIN
    Ili9340Dc = 25,
    Ili9340Rst = 18,
}


pub fn set_pin_function(pin: Pin, func: Function)
{
    let pin_number     = pin as usize;
    let gpf_sel_number = pin_number / 10;
    let gpf_sel_addr   = Addr::GpioBase.to_usize() + (gpf_sel_number * 0x4);
    let gpf_sel_ptr    = gpf_sel_addr as *mut u32;
    let shift          = 3 * (pin_number % 10);
    let set_value      = (func as u32) << shift;

    unsafe {
        let current_value = core::intrinsics::volatile_load(gpf_sel_ptr);
        let new_value = (current_value & !(7 << shift)) | set_value;
        core::intrinsics::volatile_store(gpf_sel_ptr, new_value);
    }
}


pub fn write_output_pin(pin: Pin, mode: Output)
{
    let pin_number = pin as usize;
    let reg_addr =
        if pin_number <= 31 {
            match mode {
                Output::Set   => Addr::GpioGpSet0.to_usize(),
                Output::Clear => Addr::GpioGpClr0.to_usize(),
            }
        } else {
            match mode {
                Output::Set   => Addr::GpioGpSet1.to_usize(),
                Output::Clear => Addr::GpioGpClr1.to_usize(),
            }
        };
    let reg_ptr = reg_addr as *mut u32;
    let shift = pin_number % 32;

    unsafe {
        let current_value = core::intrinsics::volatile_load(reg_ptr);
        let new_value = (current_value & !(1 << shift)) | (1 << shift);
        core::intrinsics::volatile_store(reg_ptr, new_value);
    }
}
