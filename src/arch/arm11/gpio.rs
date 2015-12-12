//! GPIO module.
//! http://www.valvers.com/open-software/raspberry-pi/step01-bare-metal-programming-in-cpt1/
//! https://www.raspberrypi.org/documentation//hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf
//!  LED ON, Zero -> CLR
//!        , Pi 1 B+ -> SET

#[allow(dead_code)]
pub enum Function {
    Input = 0b000,
    Output = 0b001,
}

pub enum Output {
    CLEAR,
    SET,
}

#[allow(dead_code)]
enum PeripheralAddr {
#[cfg(any(RPi_a, RPi_aplus, RPi_bplus, RPi_zero))]
    Base = 0x20000000,
#[cfg(any(RPi2_b))]
    Base = 0x3F000000,

    GpioBase,
    GpioGpfSel0, // GPIO Function Select 0 to 5, read and write.
    GpioGpfSel1,
    GpioGpfSel2,
    GpioGpfSel3,
    GpioGpfSel4,
    GpioGpfSel5,
    GpioGpSet0,  // GPIO Pin Output Set 0 to 1, only write.
    GpioGpSet1,
    GpioGpClr0,  // GPIO Pin Output Clear 0 to 1, only write.
    GpioGpClr1,
}


pub enum PinNumber {
#[cfg(any(RPi_a, RPi_aplus))]
    OkLed = 16,
#[cfg(any(RPi_zero, RPi_bplus, RPi2_b))]
    OkLed = 47,
}


impl PeripheralAddr {
    fn into(self) -> usize
    {
        let base      = PeripheralAddr::Base as usize;
        let gpio_base = base + 0x200000;
        match self {
            PeripheralAddr::Base        => base,
            PeripheralAddr::GpioBase    => gpio_base,
            PeripheralAddr::GpioGpfSel0 => gpio_base + 0x00,
            PeripheralAddr::GpioGpfSel1 => gpio_base + 0x04,
            PeripheralAddr::GpioGpfSel2 => gpio_base + 0x08,
            PeripheralAddr::GpioGpfSel3 => gpio_base + 0x0C,
            PeripheralAddr::GpioGpfSel4 => gpio_base + 0x10,
            PeripheralAddr::GpioGpfSel5 => gpio_base + 0x14,
            PeripheralAddr::GpioGpSet0  => gpio_base + 0x1C,
            PeripheralAddr::GpioGpSet1  => gpio_base + 0x20,
            PeripheralAddr::GpioGpClr0  => gpio_base + 0x28,
            PeripheralAddr::GpioGpClr1  => gpio_base + 0x2C,
        }
    }
}


pub fn set_pin_function(pin_number: PinNumber, func: Function)
{
    let n              = pin_number as usize;
    let gpf_sel_number = n / 10;
    let gpf_sel_addr   = PeripheralAddr::GpioBase.into() + gpf_sel_number * 0x4;
    let set_value      = (func as u32) << (3 * (n % 10));

    unsafe {
        *(gpf_sel_addr as *mut u32) = set_value;
    }
}


pub fn write_output_pin(pin_number: PinNumber, mode: Output)
{
    let n = pin_number as usize;
    let register =
        if n <= 31 {
            match mode {
                Output::SET   => PeripheralAddr::GpioGpSet0.into(),
                Output::CLEAR => PeripheralAddr::GpioGpClr0.into(),
            }
        } else {
            match mode {
                Output::SET   => PeripheralAddr::GpioGpSet1.into(),
                Output::CLEAR => PeripheralAddr::GpioGpClr1.into(),
            }
        };

    let addr = register;
    unsafe {
        *(addr as *mut u32) = 1 << (n % 32);
    }
}

#[inline(never)]
pub fn dummy_wait() {
    unsafe {
    asm!("
            mov r2,#0x90000
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

pub fn init()
{
    set_pin_function(PinNumber::OkLed, Function::Output);

    loop {
        write_output_pin(PinNumber::OkLed, Output::CLEAR);
        dummy_wait();
        write_output_pin(PinNumber::OkLed, Output::SET);
        dummy_wait();
    }
}
