//! This crate contains stuffs about computer graphic.
//!
//! This includes display (text and visual) object.

/// This struct represents any position of 2d-coordinate.
#[derive(PartialEq, Eq, Debug)]
struct Position(isize, isize);

pub enum Color {
    RGB(i8, i8, i8),
    BLACK,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHT_GRAY,
    DARK_GRAY,
    LIGHT_BLUE,
    LIGHT_GREEN,
    LIGHT_CYAN,
    LIGHT_RED,
    LIGHT_MAGENTA,
    YELLOW,
    WHITE,
}


/// Display trait for any display.
///
/// This trait is abstract interface for display.
pub trait Display {
    fn color_background(&self) -> &Color;
    fn set_color_background(&mut self, Color) -> &Self;
    fn color_foreground(&self) -> &Color;
    fn set_color_foreground(&mut self, Color) -> &Self;
}


/// Text display struct to represent text display connected to the computer.
pub struct CharacterDisplay {
    vram_addr: usize,
    color_background: Color,
    color_foreground: Color,
    current_position: Position,
}


impl Default for CharacterDisplay {
    fn default() -> CharacterDisplay
    {
        CharacterDisplay {
            vram_addr: 0,
            color_background: Color::BLACK,
            color_foreground: Color::GREEN,
            current_position: Position(0, 0),
        }
    }
}


impl CharacterDisplay {
    pub fn new(vram_addr: usize) -> CharacterDisplay
    {
        CharacterDisplay {
            vram_addr: vram_addr,
            ..Default::default()
        }
    }

    fn color(c: &Color) -> i8 {
        match *c {
            Color::BLACK         => 0x0,
            Color::BLUE          => 0x1,
            Color::GREEN         => 0x2,
            Color::CYAN          => 0x3,
            Color::RED           => 0x4,
            Color::MAGENTA       => 0x5,
            Color::BROWN         => 0x6,
            Color::LIGHT_GRAY    => 0x7,
            Color::DARK_GRAY     => 0x8,
            Color::LIGHT_BLUE    => 0x9,
            Color::LIGHT_GREEN   => 0xA,
            Color::LIGHT_CYAN    => 0xB,
            Color::LIGHT_RED     => 0xC,
            Color::LIGHT_MAGENTA => 0xD,
            Color::YELLOW        => 0xE,
            Color::WHITE         => 0xF,
            // TODO: add error handling RGB
            _                    => 0x1,
        }
    }
}


impl Display for CharacterDisplay {
    fn color_background(&self) -> &Color
    {
        &self.color_background
    }

    fn set_color_background(&mut self, bg: Color) -> &Self
    {
        self.color_background = bg;
        self
    }

    fn color_foreground(&self) -> &Color
    {
        &self.color_foreground
    }

    fn set_color_foreground(&mut self, fg: Color) -> &Self
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
    fn test_create_character_display() {
        let c_disp = CharacterDisplay::new(0xB8000);
        assert_eq!(c_disp.current_position, Position(0, 0));
    }

    #[test]
    fn test_set_bg_fg_color()
    {
        let mut c_disp = CharacterDisplay::new(0xB8000);

        c_disp.set_color_background(Color::RED);
        let color_code = CharacterDisplay::color(c_disp.color_background());
        assert_eq!(color_code, 0x4);

        c_disp.set_color_background(Color::LIGHT_BLUE);
        let color_code = CharacterDisplay::color(c_disp.color_background());
        assert_eq!(color_code, 0x9);
    }
}
