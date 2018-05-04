use list::Node;

pub trait Object: Sized + Node<Self> {
    fn mark_used(&mut self);
    fn mark_free(&mut self);
    fn is_used(&self) -> bool;
    fn order(&self) -> usize;
    fn set_order(&mut self, order: usize);

    fn size(&self) -> usize {
        1 << self.order()
    }
}
