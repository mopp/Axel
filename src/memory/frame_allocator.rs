use crate::memory::Frame;

pub trait FrameAllocator {
    fn alloc_one(&mut self) -> Option<Frame>;
    fn free(&mut self, f: Frame);
}
