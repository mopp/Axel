//! This module provides peripheral addresses based on BCM2835.

use core;

#[allow(dead_code)]
#[repr(usize)]
pub enum Addr {
    Base,

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

    AuxIrq,             // Auxiliary Interrupt status   size: 3
    AuxEnables,         // Auxiliary enables            size: 3
    AuxMuIoReg,         // Mini Uart I/O Data           size: 8
    AuxMuIerReg,        // Mini Uart Interrupt Enable   size: 8
    AuxMuIirReg,        // Mini Uart Interrupt Identify size: 8
    AuxMuLcrReg,        // Mini Uart Line Control       size: 8
    AuxMuMcrReg,        // Mini Uart Modem Control      size: 8
    AuxMuLsrReg,        // Mini Uart Line Status        size: 8
    AuxMuMsrReg,        // Mini Uart Modem Status       size: 8
    AuxMuScratch,       // Mini Uart Scratch            size: 8
    AuxMuCntlReg,       // Mini Uart Extra Control      size: 8
    AuxMuStatReg,       // Mini Uart Extra Status       size: 32
    AuxMuBaudReg,       // Mini Uart Baudrate           size: 16
    AuxSpi1Cntl0Reg,    // SPI1 Control register0       size: 32
    AuxSpi1Cntl1Reg,    // SPI1 Control register1       size: 8
    AuxSpi1StatReg,     // SPI1 Status                  size: 32
    AuxSpi1IoReg,       // SPI1 Data                    size: 32
    AuxSpi1PeekReg,     // SPI1 Peek                    size: 16
    AuxSpi2Cntl0Reg,    // SPI2 Control register0       size: 32
    AuxSpi2Cntl1Reg,    // SPI2 Control register1       size: 8
    AuxSpi2StatReg,     // SPI2 Status                  size: 32
    AuxSpi2IoReg,       // SPI2 Data                    size: 32
    AuxSpi2PeekReg,     // SPI2 Peek                    size: 16
}


impl Addr {
    pub fn to_usize(&self) -> usize
    {
        let base: usize;
        if cfg!(any(RPi_a, RPi_aplus, RPi_bplus, RPi_zero)) {
            base = 0x20000000;
        } else {
            base = 0x3F000000;
        }

        let gpio_base = base + 0x00200000;
        let spi_base  = base + 0x00204000;
        let aux_base  = base + 0x00215000;
        match *self {
            Addr::Base             => base,
            Addr::GpioBase         => gpio_base,
            Addr::GpioGpfSel0      => gpio_base + 0x00,
            Addr::GpioGpfSel1      => gpio_base + 0x04,
            Addr::GpioGpfSel2      => gpio_base + 0x08,
            Addr::GpioGpfSel3      => gpio_base + 0x0C,
            Addr::GpioGpfSel4      => gpio_base + 0x10,
            Addr::GpioGpfSel5      => gpio_base + 0x14,
            Addr::GpioGpSet0       => gpio_base + 0x1C,
            Addr::GpioGpSet1       => gpio_base + 0x20,
            Addr::GpioGpClr0       => gpio_base + 0x28,
            Addr::GpioGpClr1       => gpio_base + 0x2C,
            Addr::Spi0Base         => spi_base,
            Addr::Spi0RegisterCs   => spi_base + 0x00,
            Addr::Spi0RegisterFifo => spi_base + 0x04,
            Addr::Spi0RegisterClk  => spi_base + 0x08,
            Addr::Spi0RegisterDlen => spi_base + 0x0C,
            Addr::Spi0RegisterLtoh => spi_base + 0x10,
            Addr::Spi0RegisterDc   => spi_base + 0x14,
            Addr::AuxIrq           => aux_base + 0x00,
            Addr::AuxEnables       => aux_base + 0x04,
            Addr::AuxMuIoReg       => aux_base + 0x40,
            Addr::AuxMuIerReg      => aux_base + 0x44,
            Addr::AuxMuIirReg      => aux_base + 0x48,
            Addr::AuxMuLcrReg      => aux_base + 0x4C,
            Addr::AuxMuMcrReg      => aux_base + 0x50,
            Addr::AuxMuLsrReg      => aux_base + 0x54,
            Addr::AuxMuMsrReg      => aux_base + 0x58,
            Addr::AuxMuScratch     => aux_base + 0x5C,
            Addr::AuxMuCntlReg     => aux_base + 0x60,
            Addr::AuxMuStatReg     => aux_base + 0x64,
            Addr::AuxMuBaudReg     => aux_base + 0x68,
            Addr::AuxSpi1Cntl0Reg  => aux_base + 0x80,
            Addr::AuxSpi1Cntl1Reg  => aux_base + 0x84,
            Addr::AuxSpi1StatReg   => aux_base + 0x88,
            Addr::AuxSpi1IoReg     => aux_base + 0x90,
            Addr::AuxSpi1PeekReg   => aux_base + 0x94,
            Addr::AuxSpi2Cntl0Reg  => aux_base + 0xC0,
            Addr::AuxSpi2Cntl1Reg  => aux_base + 0xC4,
            Addr::AuxSpi2StatReg   => aux_base + 0xC8,
            Addr::AuxSpi2IoReg     => aux_base + 0xD0,
            Addr::AuxSpi2PeekReg   => aux_base + 0xD4,
        }
    }

    pub fn from_usize(v: usize) -> Addr
    {
        unsafe { core::mem::transmute(v) }
    }
}
