//! `BuddyBuddyFrame` defines a physical frame struct for buddy system.
use super::frame::SIZE;

#[derive(Copy, Clone, PartialEq, Debug)]
pub enum State {
    Free,
    Alloc,
}


#[repr(C, packed)]
pub struct BuddyFrame {
    order: u8,
    state: State,
}


impl BuddyFrame {
    pub fn order(&self) -> usize {
        self.order as usize
    }


    pub fn set_order(&mut self, order: usize) {
        use core;
        debug_assert!(order <= (core::u8::MAX as usize));
        self.order = order as u8;
    }


    pub fn state(&self) -> State {
        self.state
    }


    pub fn set_state(&mut self, state: State) {
        self.state = state;
    }


    pub fn is_alloc(&self) -> bool {
        self.state == State::Alloc
    }


    pub fn is_free(&self) -> bool {
        !self.is_alloc()
    }


    pub fn size(&self) -> usize {
        SIZE * (1 << self.order)
    }
}


impl Default for BuddyFrame {
    fn default() -> BuddyFrame {
        BuddyFrame {
            order: 0,
            state: State::Free,
        }
    }
}


#[cfg(test)]
mod test {
    use super::{BuddyFrame, State};


    #[test]
    #[should_panic]
    fn test_frame() {
        let mut frame = BuddyFrame {
            order: 0,
            state: State::Free,
        };

        frame.set_order(300);
    }
}
