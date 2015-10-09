#![feature(core)]
#![feature(no_std)]
#![no_std]

use core::cell::Cell;

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

pub struct Graphic {
    vram_addr: usize,
    is_text_mode: bool,
    pub color_background: Cell<u8>,
    pub color_foreground: Cell<u8>,
}


impl Graphic {
    pub fn new(is_text: bool, vaddr: usize) -> Graphic
    {
        let default_bg = if is_text == true { COLOR_TEXT_BLACK } else { 0 };
        let default_fg = if is_text == true { COLOR_TEXT_GREEN } else { 0 };
        Graphic {
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


// Test codes.
#[cfg(test)]
#[macro_use]
extern crate std;

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn get_one_pixel() {
        let graphic = Graphic::new(true, 0xB800);
        graphic.change_color(COLOR_TEXT_BLUE, COLOR_TEXT_GREEN);
        assert_eq!(graphic.get_one_pixel(&'A'), 0x1241);
        println!("\t0x{:x}", graphic.get_one_pixel(&'A'));
    }
}
