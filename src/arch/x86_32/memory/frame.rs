/// Frame module.

// 4KB, Page size of x86_32.
pub const SIZE: usize = 4096;

pub enum FrameState {
    Free,
    Alloc,
}


pub struct Frame {
    pub status: FrameState,
    pub order: usize,
}
