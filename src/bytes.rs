pub trait Bytes {
    fn kb(&self) -> usize;

    fn mb(&self) -> usize {
        self.kb() / 1024
    }

    fn gb(&self) -> usize {
        self.mb() / 1024
    }
}

impl Bytes for usize {
    fn kb(&self) -> usize {
        self / 1024
    }
}
