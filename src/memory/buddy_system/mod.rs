pub mod allocator;

pub use self::allocator::BuddyAllocator;

pub trait Object {
    fn reset_link(&mut self);
    fn mark_used(&mut self);
    fn mark_free(&mut self);
    fn is_used(&self) -> bool;
    fn order(&self) -> usize;
    fn set_order(&mut self, order: usize);

    fn size(&self) -> usize {
        1 << self.order()
    }
}
