//! `Frame` define a physical memory region.
//! The size is 4096 and it corresponds page size.
pub const SIZE: usize = 4096;

pub struct Frame {
    number: usize,
}
