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
    ili9340::init();
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


/// Read and write data using polling.
/// Referred from BCM2835 manual,
/// >10.6.1 Polled
/// >a) Set CS, CPOL, CPHA as required and set TA = 1.
/// >b) Poll TXD writing bytes to SPI_FIFO, RXD reading bytes from SPI_FIFO until all data written.
/// >c) Poll DONE until it goes to 1.
/// >d) Set TA = 0.
fn spi_transfer(trans_value: u8) -> u8
{
    // Clear TX and RX FIFO and TA(transfer active) = 1..
    let new_config = bitmask!(Addr::Spi0RegisterCs.load::<u32>(), 0b1011 << 4, 0b1011 << 4);
    Addr::Spi0RegisterCs.store::<u32>(new_config);

    // Polling TXD.
    loop {
        let cs_value = Addr::Spi0RegisterCs.load:: <u32>();
        let txd_value = cs_value & (1 << 18);
        if txd_value != 0 {
            break;
        }
    }

    // Write to FIFO
    Addr::Spi0RegisterFifo.store::<u32>(trans_value as u32);

    // Polling DONE
    loop {
        let cs_value = Addr::Spi0RegisterCs.load:: <u32>();
        let done_value = cs_value & (1 << 16);
        if done_value != 0 {
            break;
        }
    }

    // Read from FIFO
    let read_value = (Addr::Spi0RegisterFifo.load::<u32>() & 0xFF) as u8;

    // Set TA = 0.
    let new_config = bitmask!(Addr::Spi0RegisterCs.load::<u32>(), 1 << 7, 0 << 7);
    Addr::Spi0RegisterCs.store::<u32>(new_config);

    read_value
}


/// Module for ILI9340 IC.
mod ili9340 {
    use super::super::gpio;
    use super::super::addr::Addr;
    use super::super::timer;

    fn init_lcd() {
        write_command(0x01);
        timer::wait(5);

        write_command(0x28);

        write_command(0xEF);
        write_data(0x03);
        write_data(0x80);
        write_data(0x02);

        write_command(0xCF);
        write_data(0x00);
        write_data(0xC1);
        write_data(0x30);

        write_command(0xED);
        write_data(0x64);
        write_data(0x03);
        write_data(0x12);
        write_data(0x81);

        write_command(0xE8);
        write_data(0x85);
        write_data(0x00);
        write_data(0x78);

        write_command(0xCB);
        write_data(0x39);
        write_data(0x2C);
        write_data(0x00);
        write_data(0x34);
        write_data(0x02);

        write_command(0xF7);
        write_data(0x20);

        write_command(0xEA);
        write_data(0x00);
        write_data(0x00);

        write_command(0xC0);
        write_data(0x23);

        write_command(0xC1);
        write_data(0x10);

        write_command(0xC5);
        write_data(0x3e);
        write_data(0x28);

        write_command(0xC7);
        write_data(0x86);

        write_command(0x3A);
        write_data(0x55);

        write_command(0xB1);
        write_data(0x00);
        write_data(0x18);

        write_command(0xB6);
        write_data(0x08);
        write_data(0x82);
        write_data(0x27);

        write_command(0xF2);
        write_data(0x00);

        write_command(0x26);
        write_data(0x01);

        write_command(0xE0);
        write_data(0x0F);
        write_data(0x31);
        write_data(0x2B);
        write_data(0x0C);
        write_data(0x0E);
        write_data(0x08);
        write_data(0x4E);
        write_data(0xF1);
        write_data(0x37);
        write_data(0x07);
        write_data(0x10);
        write_data(0x03);
        write_data(0x0E);
        write_data(0x09);
        write_data(0x00);

        write_command(0xE1);
        write_data(0x00);
        write_data(0x0E);
        write_data(0x14);
        write_data(0x03);
        write_data(0x11);
        write_data(0x07);
        write_data(0x31);
        write_data(0xC1);
        write_data(0x48);
        write_data(0x08);
        write_data(0x0F);
        write_data(0x0C);
        write_data(0x31);
        write_data(0x36);
        write_data(0x0F);

        write_command(0x11);
        timer::wait(100);

        write_command(0x29);
        timer::wait(20);
    }

    pub fn init()
    {
        // Set SPI mode in mode0.
        // mode0 has referred CPOL = 0 and CPHA = 0.
        let new_config = bitmask!(Addr::Spi0RegisterCs.load::<u32>(), 0b11 << 2, 0);
        Addr::Spi0RegisterCs.store::<u32>(new_config);

        // Set clock divider.
        Addr::Spi0RegisterClk.store::<u32>(8);

        // Select SPI slave.
        super::select_chip(super::Chip::Ce0);

        // Set GPIO pins that are used by ILI9340.
        gpio::set_pin_function(gpio::Pin::Ili9340Dc, gpio::Function::Output);
        gpio::set_pin_function(gpio::Pin::Ili9340Rst, gpio::Function::Output);

        init_lcd();

        for i in 0..50 {
            for j in 0..50 {
                draw(i, i, j, j);
            }
        }
    }

    fn draw(x0: u8, x1: u8, y0: u8, y1: u8)
    {
        write_command(0x2A);  // set column(x) address
        write_data(0);
        write_data(x0);
        write_data(0);
        write_data(x1);

        write_command(0x2B);  // set Page(y) address
        write_data(0);
        write_data(y0);
        write_data(0);
        write_data(y1);

        write_command(0x2C);  // Memory Write
        write_data(0xF8);
        write_data(0x00);
    }


    fn write_command(command: u8)
    {
        // Set register for writing command.
        gpio::write_output_pin(gpio::Pin::Ili9340Dc, gpio::Output::Clear);
        super::spi_transfer(command);
    }


    fn write_data(data: u8)
    {
        // Set register for writing data.
        gpio::write_output_pin(gpio::Pin::Ili9340Dc, gpio::Output::Set);
        super::spi_transfer(data);
    }
}
