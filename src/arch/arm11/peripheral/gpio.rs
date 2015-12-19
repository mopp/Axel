//! GPIO module.
//! http://www.valvers.com/open-software/raspberry-pi/step01-bare-metal-programming-in-cpt1/
//! https://www.raspberrypi.org/documentation//hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf
//!  LED ON, Zero -> CLR
//!        , Pi 1 B+ -> SET

use super::addr::Addr;


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
    CLEAR,
    SET,
}


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

    Ili9340Dc = 17,
    Ili9340Res = 18,
}


pub fn set_pin_function(pin_number: Pin, func: Function)
{
    let n                  = pin_number as usize;
    let gpf_sel_number:usize = n / 10;
    let gpf_sel_addr  :usize = Addr::GpioBase.to_usize() + (gpf_sel_number * 0x4);
    let set_value      = (func as u32) << (3 * (n % 10));

    unsafe {
        *(gpf_sel_addr as *mut u32) = set_value;
    }
}


pub fn write_output_pin(pin_number: Pin, mode: Output)
{
    let n = pin_number as usize;
    let register =
        if n <= 31 {
            match mode {
                Output::SET   => Addr::GpioGpSet0.to_usize(),
                Output::CLEAR => Addr::GpioGpClr0.to_usize(),
            }
        } else {
            match mode {
                Output::SET   => Addr::GpioGpSet1.to_usize(),
                Output::CLEAR => Addr::GpioGpClr1.to_usize(),
            }
        };

    let addr = register;
    unsafe {
        *(addr as *mut u32) = 1 << (n % 32);
    }
}


#[inline(never)]
pub fn dummy_wait()
{
    unsafe {
        asm!("
            mov r2,#0xF0000
        inner_loop:
            sub r2, #1
            eor r1, r2, r1
            cmp r2, #0
            bne inner_loop"
            :
            :
            : "r1", "r2"
            );
    }
}


pub unsafe fn set_bit(addr: usize, val: u32, clobbered_mask: u32) {
    let ptr = addr as *mut u32;
    let mut current_val = *ptr;

    // Keep Not write value.
    let new_val = (current_val & !clobbered_mask) | (val & clobbered_mask);
    *ptr = new_val;
}
