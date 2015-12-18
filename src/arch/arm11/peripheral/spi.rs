//! This module provides SPI util functions.

use super::gpio;
use super::addr::Addr;


/// Only the SPI0 controller is available on the header pin.
pub fn init_spi0() {
    // Set GPIO function for SPI0.
    gpio::set_pin_function(gpio::GpioPin::Spi0Ce1N, gpio::Function::Alternate0);
    gpio::set_pin_function(gpio::GpioPin::Spi0Ce0N, gpio::Function::Alternate0);
    gpio::set_pin_function(gpio::GpioPin::Spi0Miso, gpio::Function::Alternate0);
    gpio::set_pin_function(gpio::GpioPin::Spi0Mosi, gpio::Function::Alternate0);
    gpio::set_pin_function(gpio::GpioPin::Spi0Sclk, gpio::Function::Alternate0);


    // Set SPI0 CS register to all zero.
    unsafe {
        *(Addr::Spi0RegisterCs.to_usize() as *mut u32) = 0;

        // Clear TX and RX FIFO.
        // With memory barrier.
        // asm!("dmb");
        *(Addr::Spi0RegisterCs.to_usize() as *mut u32) = 0x00000030;

        // Set data mode.
        // With memory barrier.
        // asm!("dmb");
        *(Addr::Spi0RegisterCs.to_usize() as *mut u32) = 0x00000030;
    }
}


unsafe fn spi_transfer(val: u8) -> u8 {
    // Clear TX and RX fifos.
    gpio::set_bit(Addr::Spi0RegisterCs.to_usize(), 0x00000030, 0x00000030);

    // Set TA = 1.
    gpio::set_bit(Addr::Spi0RegisterCs.to_usize(), 1 << 7, 1 << 7);

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
    gpio::set_bit(Addr::Spi0RegisterCs.to_usize(), 0, 1 << 7);

    read_data
}


/// For ILI9340 IC.
fn ili9340_write_command(command: u8) {
    gpio::write_output_pin(gpio::GpioPin::Ili9340Dc, gpio::Output::CLEAR);
    unsafe {
        spi_transfer(command);
    }
    gpio::write_output_pin(gpio::GpioPin::Ili9340Dc, gpio::Output::SET);
}

// SPI Write Data
// D/C=HIGH then,write data(8bit)
fn ili9340_write_data(data: u8)
{
    gpio::write_output_pin(gpio::GpioPin::Ili9340Dc, gpio::Output::SET);
    unsafe {
        spi_transfer(data);
    }
}
