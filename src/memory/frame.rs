//! `Frame` define a physical memory region.
//! The size is 4096 and it corresponds page size.
pub const SIZE: usize = 4096;

use intrusive_collections::{LinkedListLink, UnsafeRef};
use memory::buddy_system::Object;
use VirtualAddress;

#[derive(Clone, Debug, PartialEq, Eq)]
enum State {
    Used,
    Free,
}

#[derive(Clone, Debug)]
pub struct Frame {
    link: LinkedListLink,
    number: usize,
    order: usize,
    state: State,
}

intrusive_adapter!(pub FrameAdapter = UnsafeRef<Frame>: Frame { link: LinkedListLink });

impl Frame {
    pub fn from_addr(addr: VirtualAddress) -> Frame {
        Frame {
            // FIXME
            link: LinkedListLink::new(),
            order: 0,
            state: State::Free,
            number: addr / SIZE,
        }
    }

    pub fn level4_index(&self) -> usize {
        (self.number >> 27) & 0o777
    }

    pub fn level3_index(&self) -> usize {
        (self.number >> 18) & 0o777
    }

    pub fn level2_index(&self) -> usize {
        (self.number >> 9) & 0o777
    }

    pub fn level1_index(&self) -> usize {
        (self.number >> 0) & 0o777
    }
}

impl Object for Frame {
    fn reset_link(&mut self) {
        self.link = LinkedListLink::new();
    }

    fn mark_used(&mut self) {
        self.state = State::Used;
    }

    fn mark_free(&mut self) {
        self.state = State::Free;
    }

    fn is_used(&self) -> bool {
        self.state == State::Free
    }

    fn order(&self) -> usize {
        self.order
    }

    fn set_order(&mut self, order: usize) {
        self.order = order;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_index() {
        let f = Frame::from_addr(0x00000);

        assert_eq!(f.level4_index(), 0);
        assert_eq!(f.level3_index(), 0);
        assert_eq!(f.level2_index(), 0);
        assert_eq!(f.level1_index(), 0);

        let f = Frame::from_addr(0o456_555_456_123_0000);
        assert_eq!(f.level4_index(), 0o456);
        assert_eq!(f.level3_index(), 0o555);
        assert_eq!(f.level2_index(), 0o456);
        assert_eq!(f.level1_index(), 0o123);
    }
}
