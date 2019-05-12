use super::super::Frame;

pub trait FrameAllocator {
    fn alloc_one() -> Frame;
    fn alloc(_: usize) -> Frame;
    fn free(_: Frame);
}
