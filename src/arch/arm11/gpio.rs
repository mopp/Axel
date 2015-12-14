//! GPIO module.
//! http://www.valvers.com/open-software/raspberry-pi/step01-bare-metal-programming-in-cpt1/
//! https://www.raspberrypi.org/documentation//hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf
//!  LED ON, Zero -> CLR
//!        , Pi 1 B+ -> SET


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

    Spi0Base,
    Spi0RegisterCs,
    Spi0RegisterFifo,
    Spi0RegisterClk,
    Spi0RegisterDlen,
    Spi0RegisterLtoh,
    Spi0RegisterDc,
}


pub enum GpioPin {
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


impl PeripheralAddr {
    fn into(self) -> usize
    {
        let base      = PeripheralAddr::Base as usize;
        let gpio_base = base + 0x00200000;
        let spi_base  = base + 0x00204000;
        match self {
            PeripheralAddr::Base             => base,
            PeripheralAddr::GpioBase         => gpio_base,
            PeripheralAddr::GpioGpfSel0      => gpio_base + 0x00,
            PeripheralAddr::GpioGpfSel1      => gpio_base + 0x04,
            PeripheralAddr::GpioGpfSel2      => gpio_base + 0x08,
            PeripheralAddr::GpioGpfSel3      => gpio_base + 0x0C,
            PeripheralAddr::GpioGpfSel4      => gpio_base + 0x10,
            PeripheralAddr::GpioGpfSel5      => gpio_base + 0x14,
            PeripheralAddr::GpioGpSet0       => gpio_base + 0x1C,
            PeripheralAddr::GpioGpSet1       => gpio_base + 0x20,
            PeripheralAddr::GpioGpClr0       => gpio_base + 0x28,
            PeripheralAddr::GpioGpClr1       => gpio_base + 0x2C,
            PeripheralAddr::Spi0Base         => spi_base,
            PeripheralAddr::Spi0RegisterCs   => spi_base + 0x00,
            PeripheralAddr::Spi0RegisterFifo => spi_base + 0x04,
            PeripheralAddr::Spi0RegisterClk  => spi_base + 0x08,
            PeripheralAddr::Spi0RegisterDlen => spi_base + 0x0C,
            PeripheralAddr::Spi0RegisterLtoh => spi_base + 0x10,
            PeripheralAddr::Spi0RegisterDc   => spi_base + 0x14,
        }
    }
}


pub fn set_pin_function(pin_number: GpioPin, func: Function)
{
    let n              = pin_number as usize;
    let gpf_sel_number = n / 10;
    let gpf_sel_addr   = PeripheralAddr::GpioBase.into() + gpf_sel_number * 0x4;
    let set_value      = (func as u32) << (3 * (n % 10));

    unsafe {
        *(gpf_sel_addr as *mut u32) = set_value;
    }
}


pub fn write_output_pin(pin_number: GpioPin, mode: Output)
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


unsafe fn set_bit(addr: usize, val: u32, clobbered_mask: u32) {
    let ptr = addr as *mut u32;
    let mut current_val = *ptr;

    // Keep Not write value.
    let new_val = (current_val & !clobbered_mask) | (val & clobbered_mask);
    *ptr = new_val;
}


fn init_spi() {
    // Set GPIO function for SPI0.
    set_pin_function(GpioPin::Spi0Ce1N, Function::Alternate0);
    set_pin_function(GpioPin::Spi0Ce0N, Function::Alternate0);
    set_pin_function(GpioPin::Spi0Miso, Function::Alternate0);
    set_pin_function(GpioPin::Spi0Mosi, Function::Alternate0);
    set_pin_function(GpioPin::Spi0Sclk, Function::Alternate0);

    // Set SPI0 CS register to all zero.
    unsafe {
        *(PeripheralAddr::Spi0RegisterCs.into() as *mut u32) = 0;

        // Clear TX and RX FIFO.
        // With memory barrier.
        // asm!("dmb");
        *(PeripheralAddr::Spi0RegisterCs.into() as *mut u32) = 0x00000030;

        // Set data mode.
        // With memory barrier.
        // asm!("dmb");
        *(PeripheralAddr::Spi0RegisterCs.into() as *mut u32) = 0x00000030;
    }
}


unsafe fn spi_transfer(val: u8) -> u8 {
    // Clear TX and RX fifos.
    set_bit(PeripheralAddr::Spi0RegisterCs.into(), 0x00000030, 0x00000030);

    // Set TA = 1.
    set_bit(PeripheralAddr::Spi0RegisterCs.into(), 1 << 7, 1 << 7);

    // Wait for TXD.
    loop {
        // WTF
        // let cs_reg :u32= volatile_load(PeripheralAddr::Spi0RegisterFifo.into() as *const u32);
        let mut val = 0;
        asm!(
            "ldr r1, [r0]"
            : "={r1}"(val)
            : "{r0}"(PeripheralAddr::Spi0RegisterCs.into())
            : "volatile", "intel"
            );
        if val == 0x30 {
            break;
        }
    }

    // Write to FIFO
    *(PeripheralAddr::Spi0RegisterFifo.into() as *mut u8) = val;

    // Wait for DONE
    loop {
        // WTF
        // let cs_reg :u32= volatile_load(PeripheralAddr::Spi0RegisterFifo.into() as *const u32);
        let mut val = 0;
        asm!(
            "ldr r1, [r0]"
            : "={r1}"(val)
            : "{r0}"(PeripheralAddr::Spi0RegisterCs.into())
            : "volatile", "intel"
            );
        if val == 0x00010000 {
            break;
        }
    }

    // Write to FIFO
    let read_data = *(PeripheralAddr::Spi0RegisterFifo.into() as *mut u8);

    // Set TA = 1.
    set_bit(PeripheralAddr::Spi0RegisterCs.into(), 0, 1 << 7);

    read_data
}


/// For ILI9340 IC.
fn ili9340_write_command(command: u8) {
    write_output_pin(GpioPin::Ili9340Dc, Output::CLEAR);
    unsafe {
        spi_transfer(command);
    }
    write_output_pin(GpioPin::Ili9340Dc, Output::SET);
}

// SPI Write Data
// D/C=HIGH then,write data(8bit)
fn ili9340_write_data(data: u8)
{
    write_output_pin(GpioPin::Ili9340Dc, Output::SET);
    unsafe {
        spi_transfer(data);
    }
}


pub fn init()
{
    set_pin_function(GpioPin::OkLed, Function::Output);

    loop {
        write_output_pin(GpioPin::OkLed, Output::CLEAR);
        dummy_wait();
        write_output_pin(GpioPin::OkLed, Output::SET);
        dummy_wait();
    }
}
