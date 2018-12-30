pub mod allocator;

pub use self::allocator::BuddyAllocator;

pub trait Object {
    fn reset_link(&mut self);
    fn mark_used(&self);
    fn mark_free(&self);
    fn is_used(&self) -> bool;
    fn order(&self) -> usize;
    fn set_order(&self, order: usize);

    fn size(&self) -> usize {
        1 << self.order()
    }
}
