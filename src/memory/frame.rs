//! `Frame` define a physical memory region.
//! The size is 4096 and it corresponds page size.
pub const SIZE: usize = 4096;

use super::buddy_system::Object;
use super::VirtualAddress;
use core::cell::Cell;
use intrusive_collections::{LinkedListLink, UnsafeRef};

#[derive(Copy, Clone, Debug, PartialEq, Eq)]
enum State {
    Used,
    Free,
}

#[derive(Clone, Debug)]
pub struct Frame {
    link: LinkedListLink,
    number: usize,
    order: Cell<usize>,
    state: Cell<State>,
}

intrusive_adapter!(pub FrameAdapter = UnsafeRef<Frame>: Frame { link: LinkedListLink });

impl Frame {
    pub fn from_address(addr: VirtualAddress) -> Frame {
        Frame {
            link: LinkedListLink::new(),
            order: Cell::new(0),
            state: Cell::new(State::Free),
            number: addr / SIZE,
        }
    }

    pub fn set_number(&mut self, i: usize) {
        self.number = i;
    }

    pub fn number(&self) -> usize {
        self.number
    }

    // pub fn level4_index(&self) -> usize {
    //     (self.number >> 27) & 0o777
    // }

    // pub fn level3_index(&self) -> usize {
    //     (self.number >> 18) & 0o777
    // }

    // pub fn level2_index(&self) -> usize {
    //     (self.number >> 9) & 0o777
    // }

    // pub fn level1_index(&self) -> usize {
    //     (self.number >> 0) & 0o777
    // }

    pub fn address(&self) -> usize {
        self.number * SIZE
    }
}

impl Object for Frame {
    fn reset_link(&mut self) {
        self.link = LinkedListLink::new();
    }

    fn mark_used(&self) {
        self.state.set(State::Used);
    }

    fn mark_free(&self) {
        self.state.set(State::Free);
    }

    fn is_used(&self) -> bool {
        self.state.get() == State::Used
    }

    fn order(&self) -> usize {
        self.order.get()
    }

    fn set_order(&self, order: usize) {
        self.order.set(order);
    }
}

// #[cfg(test)]
// mod tests {
//     #[test]
//     fn test_index() {
//         let f = Frame::from_address(0x00000);
//
//         assert_eq!(f.level4_index(), 0);
//         assert_eq!(f.level3_index(), 0);
//         assert_eq!(f.level2_index(), 0);
//         assert_eq!(f.level1_index(), 0);
//
//         let f = Frame::from_address(0o456_555_456_123_0000);
//         assert_eq!(f.level4_index(), 0o456);
//         assert_eq!(f.level3_index(), 0o555);
//         assert_eq!(f.level2_index(), 0o456);
//         assert_eq!(f.level1_index(), 0o123);
//     }
// }
