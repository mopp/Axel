#![feature(no_std)]
#![no_std]


pub struct Graphic {
    is_text_mode:bool,
    vram_addr:usize,
    color_background:u8,
    color_foreground:u8,
}


impl Graphic {
    pub fn new(is_text:bool, vaddr:usize) -> Graphic
    {
        Graphic {
            is_text_mode:is_text,
            vram_addr:vaddr,
            color_background:1,
            color_foreground:2,
        }
    }


    fn get_one_pixel(&self, c:&char) -> u16
    {
        let bg = self.color_background as u16;
        let fg = self.color_foreground as u16;
        (bg << 12) | (fg << 8) | (*c as u16)
    }


    pub fn putchar(&self, c:char)
    {
        let cn = self.get_one_pixel(&c);
        self.get_one_pixel(&c);
    }
}


#[cfg(test)]
mod tests {
    use super::Graphic;

    #[test]
    fn test_get_one_pixel() {
        let graphic = Graphic::new(true, 0xB800);
        assert!(true)
    }
}
