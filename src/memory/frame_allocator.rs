use super::frame::Frame;

pub trait FrameAllocator {
    fn alloc() -> Frame;
    fn free(Frame);
}
