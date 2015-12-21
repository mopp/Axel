//! This module provides SPI util functions.

use super::gpio;
use super::addr::Addr;


#[allow(dead_code)]
enum Chip {
    Ce0,
    Ce1,
    Ce2,
}


macro_rules! bitmask (
    ($current_value: expr, $clobbered_mask: expr, $value: expr) => (
        ($current_value & !$clobbered_mask) | ($value & $clobbered_mask)
    )
);


/// Only the SPI0 controller is available on the header pin.
pub fn init_spi0()
{
    // Set GPIO functions for SPI0.
    gpio::set_pin_function(gpio::Pin::Spi0Ce0N, gpio::Function::Alternate0);
    gpio::set_pin_function(gpio::Pin::Spi0Ce1N, gpio::Function::Alternate0);
    gpio::set_pin_function(gpio::Pin::Spi0Miso, gpio::Function::Alternate0);
    gpio::set_pin_function(gpio::Pin::Spi0Mosi, gpio::Function::Alternate0);
    gpio::set_pin_function(gpio::Pin::Spi0Sclk, gpio::Function::Alternate0);

    // Set SPI0 CS register to clear.
    Addr::Spi0RegisterCs.store::<u32>(0);

    // Clear TX and RX FIFO.
    Addr::Spi0RegisterCs.store::<u32>(0b11 << 4);

    // TODO: move out.
    Ili9340::init();
}


fn select_chip(ce: Chip)
{
    let pattern :u32 = match ce {
        Chip::Ce0 => 0b00,
        Chip::Ce1 => 0b01,
        Chip::Ce2 => 0b10,
    };

    let current_config  = Addr::Spi0RegisterCs.load::<u32>();
    let new_config: u32 = bitmask!(current_config, 0b11, pattern);

    Addr::Spi0RegisterCs.store::<u32>(new_config);
}


unsafe fn spi_transfer(val: u8) -> u8
{
    // Clear TX and RX fifos.
    // gpio::set_bit(Addr::Spi0RegisterCs.to_usize(), 0x00000030, 0x00000030);

    // Set TA = 1.
    // gpio::set_bit(Addr::Spi0RegisterCs.to_usize(), 1 << 7, 1 << 7);

    // Wait for TXD.
    loop {
        // WTF
        // let cs_reg :u32= volatile_load(Addr::Spi0RegisterFifo.to_usize() as *const u32);
        let mut val = 0;
        asm!(
            "ldr r1, [r0]"
            : "={r1}"(val)
            : "{r0}"(Addr::Spi0RegisterCs.to_usize())
            :
            : "volatile", "intel"
            );
        if val == 0x30 {
            break;
        }
    }

    // Write to FIFO
    *(Addr::Spi0RegisterFifo.to_usize() as *mut u8) = val;

    // Wait for DONE
    loop {
        // WTF
        // let cs_reg :u32= volatile_load(Addr::Spi0RegisterFifo.to_usize() as *const u32);
        let mut val = 0;
        asm!(
            "ldr r1, [r0]"
            : "={r1}"(val)
            : "{r0}"(Addr::Spi0RegisterCs.to_usize())
            :
            : "volatile", "intel"
            );
        if val == 0x00010000 {
            break;
        }
    }

    // Write to FIFO
    let read_data = *(Addr::Spi0RegisterFifo.to_usize() as *mut u8);

    // Set TA = 1.
    // gpio::set_bit(Addr::Spi0RegisterCs.to_usize(), 0, 1 << 7);

    read_data
}


mod Ili9340 {
    use super::super::gpio;
    use super::super::addr::Addr;
    use super::{Chip, select_chip};

    pub fn init()
    {
        // Set SPI mode in mode0.
        // mode0 has referred CPOL = 0 and CPHA = 0.
        let new_config = bitmask!(Addr::Spi0RegisterCs.load::<u32>(), 0b11 << 2, 0);
        Addr::Spi0RegisterCs.store::<u32>(new_config);

        // Set clock divider.
        Addr::Spi0RegisterClk.store::<u32>(64);

        // Select SPI slave.
        select_chip(Chip::Ce0);

        // Set GPIO pins that are used by ILI9340.
        gpio::set_pin_function(gpio::Pin::Ili9340Dc, gpio::Function::Output);
        gpio::set_pin_function(gpio::Pin::Ili9340Rst, gpio::Function::Output);

        // Dc refers Data/Command selector.
        // 1 is data, 0 is command.
        gpio::write_output_pin(gpio::Pin::Ili9340Dc, gpio::Output::Set);

        // Reset IC.
        // Reset signal is negative logic.
        gpio::write_output_pin(gpio::Pin::Ili9340Rst, gpio::Output::Clear);
        gpio::dummy_wait();
        gpio::write_output_pin(gpio::Pin::Ili9340Rst, gpio::Output::Set);
        gpio::dummy_wait();
    }


    /// For ILI9340 IC.
    fn write_command(command: u8)
    {
        gpio::write_output_pin(gpio::Pin::Ili9340Dc, gpio::Output::Clear);
        unsafe {
            super::spi_transfer(command);
        }
        gpio::write_output_pin(gpio::Pin::Ili9340Dc, gpio::Output::Set);
    }


    // SPI Write Data
    // D/C=HIGH then,write data(8bit)
    fn write_data(data: u8)
    {
        gpio::write_output_pin(gpio::Pin::Ili9340Dc, gpio::Output::Set);
        unsafe {
            super::spi_transfer(data);
        }
    }
}
