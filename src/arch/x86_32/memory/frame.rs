/// Frame module.

// 4KB, Page size of x86_32.
pub const SIZE: usize = 4096;


#[derive(PartialEq)]
pub enum FrameState {
    Free,
    Alloc,
}


pub struct Frame {
    pub order: usize,
    pub status: FrameState,
}


impl Frame {
    pub fn size(&self) -> usize
    {
        SIZE * (1 << self.order)
    }


    pub fn is_alloc(&self) -> bool
    {
        self.status == FrameState::Alloc
    }


    #[allow(dead_code)]
    pub fn is_free(&self) -> bool
    {
        !self.is_alloc()
    }
}


impl Default for Frame {
    fn default() -> Frame
    {
        Frame {
            order: 0,
            status: FrameState::Free,
        }
    }
}
