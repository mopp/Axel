//! This crate contains stuffs about computer graphic.
//!
//! This includes display (text and visual) object.

#![feature(no_std)]
#![no_std]

pub static COLOR_TEXT_BLACK: u8         = 0x0;
pub static COLOR_TEXT_BLUE: u8          = 0x1;
pub static COLOR_TEXT_GREEN: u8         = 0x2;
pub static COLOR_TEXT_CYAN: u8          = 0x3;
pub static COLOR_TEXT_RED: u8           = 0x4;
pub static COLOR_TEXT_MAGENTA: u8       = 0x5;
pub static COLOR_TEXT_BROWN: u8         = 0x6;
pub static COLOR_TEXT_LIGHT_GRAY: u8    = 0x7;
pub static COLOR_TEXT_DARK_GRAY: u8     = 0x8;
pub static COLOR_TEXT_LIGHT_BLUE: u8    = 0x9;
pub static COLOR_TEXT_LIGHT_GREEN: u8   = 0xA;
pub static COLOR_TEXT_LIGHT_CYAN: u8    = 0xB;
pub static COLOR_TEXT_LIGHT_RED: u8     = 0xC;
pub static COLOR_TEXT_LIGHT_MAGENTA: u8 = 0xD;
pub static COLOR_TEXT_YELLOW: u8        = 0xE;
pub static COLOR_TEXT_WHITE: u8         = 0xF;


/// Graphic trait for graphical artifacts on the system.
///
/// This trait is abstract interface to do draw/print/etc processes.
pub trait Graphic {
}


/// Display trait for any display.
///
/// This trait is abstract interface for display.
pub trait Display: Graphic {
    fn vram_addr(&self) -> usize;
}


/// Text display struct to represent text display connected to the computer.
pub struct CharacterDisplay {
    vram_addr: usize,
}


impl CharacterDisplay {
    pub fn new(vram_addr: usize) -> CharacterDisplay {
        CharacterDisplay {
            vram_addr: vram_addr
        }
    }
}


impl Graphic for CharacterDisplay {
}


impl Display for CharacterDisplay {
    fn vram_addr(&self) -> usize {
        self.vram_addr
    }
}




/// Visual display struct to represent text display connected to the computer.
pub struct GraphicalDisplay {
    vram_addr: usize,
}



/*
impl Display {
    pub fn new(is_text: bool, vaddr: usize) -> Display
    {
        let default_bg = if is_text == true { COLOR_TEXT_BLACK } else { 0 };
        let default_fg = if is_text == true { COLOR_TEXT_GREEN } else { 0 };
        Display {
            vram_addr:vaddr,
            is_text_mode:is_text,
            color_background:Cell::new(default_bg),
            color_foreground:Cell::new(default_fg),
        }
    }

    pub fn change_color(&self, bg: u8, fg: u8)
    {
        self.color_background.set(bg);
        self.color_foreground.set(fg);
    }

    fn get_one_pixel(&self, c: &char) -> u16
    {
        let bg = self.color_background.get() as u16;
        let fg = self.color_foreground.get() as u16;
        (bg << 12) | (fg << 8) | (*c as u16)
    }


    pub fn putchar(&self, c: char)
    {
        let cn = self.get_one_pixel(&c);
        self.get_one_pixel(&c);
    }
}
*/



// Test codes.
#[cfg(test)]
#[macro_use]
extern crate std;

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn vram_addr() {
        let c_disp = CharacterDisplay::new(100);
        assert_eq!(c_disp.vram_addr(), 100);
        assert_eq!(c_disp.vram_addr, 100);
    }
}
