// TODO replace
struct Frame {
}

trait FrameAllocator {
    fn alloc() -> Frame;
    fn free(Frame);
}
