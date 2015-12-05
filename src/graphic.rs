//! This crate contains stuffs about computer graphic.
//!
//! This includes display (text and visual) object.
#![feature(no_std)]
#![no_std]

use core::slice;

/// This struct represents any position of 2d-coordinate.
#[derive(PartialEq, Eq, Debug)]
pub struct Position(pub isize, pub isize);

#[warn(dead_code)]
pub enum Color {
    Rgb(i8, i8, i8),
    Black,
    Blue,
    Green,
    Cyan,
    Red,
    Magenta,
    Brown,
    LightGray,
    DarkGray,
    LightBlue,
    LightGreen,
    LightCyan,
    LightRed,
    LightMagenta,
    Yellow,
    White,
}


/// Display trait for any display.
///
/// This trait is abstract interface for display.
pub trait Display {
    fn color_background(&self) -> &Color;
    fn set_color_background(&mut self, Color) -> &mut Self;
    fn color_foreground(&self) -> &Color;
    fn set_color_foreground(&mut self, Color) -> &mut Self;
}


/// Text display struct to represent text display connected to the computer.
pub struct CharacterDisplay<'a> {
    vram_addr: usize,
    vram: &'a [u8],
    current_position: Position,
    max_position: Position,
    color_background: Color,
    color_foreground: Color,
}


impl<'a> Default for CharacterDisplay<'a> {
    fn default() -> CharacterDisplay<'a>
    {
        CharacterDisplay {
            vram_addr: 0,
            vram: &[],
            current_position: Position(0, 0),
            max_position: Position(0, 0),
            color_background: Color::Black,
            color_foreground: Color::Green,
        }
    }
}


impl<'a> CharacterDisplay<'a> {
    pub fn new(vram_addr: usize, maxp: Position) -> CharacterDisplay<'a>
    {
        let Position(x, y) = maxp;
        let max_pixels_num = (x * y) as usize;
        let vram_ptr       = vram_addr as *mut u8;
        let vram           = unsafe { slice::from_raw_parts_mut(vram_ptr, max_pixels_num) };

        CharacterDisplay {
            vram_addr: vram_addr,
            vram: vram,
            max_position: maxp,
            ..Default::default()
        }
    }

    pub fn set_current_position(&mut self, pos: Position) -> &mut Self
    {
        self.current_position = pos;
        self
    }

    fn color(c: &Color) -> i8 {
        match *c {
            Color::Black        => 0x0,
            Color::Blue         => 0x1,
            Color::Green        => 0x2,
            Color::Cyan         => 0x3,
            Color::Red          => 0x4,
            Color::Magenta      => 0x5,
            Color::Brown        => 0x6,
            Color::LightGray    => 0x7,
            Color::DarkGray     => 0x8,
            Color::LightBlue    => 0x9,
            Color::LightGreen   => 0xA,
            Color::LightCyan    => 0xB,
            Color::LightRed     => 0xC,
            Color::LightMagenta => 0xD,
            Color::Yellow       => 0xE,
            Color::White        => 0xF,
            _                   => panic!("Should NOT use Rgb in CharacterDisplay."), // TODO: add error handling Rgb
        }
    }

    pub fn clear_screen(&mut self)
    {
        let Position(x, y) = self.max_position;
    }
}


impl<'a> Display for CharacterDisplay<'a> {
    fn color_background(&self) -> &Color
    {
        &self.color_background
    }

    fn set_color_background(&mut self, bg: Color) -> &mut Self
    {
        self.color_background = bg;
        self
    }

    fn color_foreground(&self) -> &Color
    {
        &self.color_foreground
    }

    fn set_color_foreground(&mut self, fg: Color) -> &mut Self
    {
        self.color_foreground = fg;
        self
    }
}


/*
/// Visual display struct to represent text display connected to the computer.
struct GraphicalDisplay {
}
*/


#[cfg(test)]
mod test {
    use super::{CharacterDisplay, Display, Color, Position};

    #[test]
    fn creating_character_display() {
        let c_disp = CharacterDisplay::new(0xB8000, Position(800, 600));
        assert_eq!(c_disp.current_position, Position(0, 0));
    }

    #[test]
    fn setting_bg_fg_color()
    {
        let mut c_disp = CharacterDisplay::new(0xB8000, Position(800, 600));

        c_disp.set_color_background(Color::Red).set_color_foreground(Color::LightBlue);
        let color_code = CharacterDisplay::color(c_disp.color_background());
        assert_eq!(color_code, 0x4);

        let color_code = CharacterDisplay::color(c_disp.color_foreground());
        assert_eq!(color_code, 0x9);
    }

    #[test]
    #[should_panic]
    fn setting_wrong_color() {
        CharacterDisplay::color(&Color::Rgb(0, 0, 0));
    }
}
