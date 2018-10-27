use core::marker::PhantomData;
use core::ptr;
use core::ptr::{NonNull, Unique};

pub trait Node<T: Node<T>> {
    fn set_next(&mut self, Option<NonNull<Self>>);
    fn set_prev(&mut self, Option<NonNull<Self>>);
    fn next(&self) -> Option<NonNull<Self>>;
    fn prev(&self) -> Option<NonNull<Self>>;

    fn init(&mut self) {
        self.set_next(None);
        self.set_prev(None);
    }

    fn count(&self) -> usize {
        let mut count = 1;
        let mut node;

        // Count the nodes to forward.
        node = self.next();
        loop {
            if let Some(next) = node {
                node = unsafe { next.as_ref().next() };
                count += 1;
            } else {
                break;
            }
        }

        // Count the nodes to backward.
        node = self.prev();
        loop {
            if let Some(prev) = node {
                node = unsafe { prev.as_ref().prev() };
                count += 1;
            } else {
                break;
            }
        }

        count
    }

    fn detach(&mut self) {
        match (self.prev(), self.next()) {
            (Some(mut prev), Some(mut next)) => {
                unsafe {
                    prev.as_mut().set_next(Some(next));
                    next.as_mut().set_prev(Some(prev));
                }
                self.init();
            }
            (None, None) => {
                // It was detached already.
            }
            (None, _) => {
                // Maybe it is head node in list.
            }
            (_, None) => {
                // Maybe it is tail node in list.
            }
        }
    }
}

pub struct LinkedList<T: Node<T>> {
    head: Option<NonNull<T>>,
    tail: Option<NonNull<T>>,
}

impl<T: Node<T>> LinkedList<T> {
    pub fn new() -> LinkedList<T> {
        LinkedList { head: None, tail: None }
    }

    pub fn head(&self) -> Option<&T> {
        self.head.map(|head| unsafe { &*head.as_ptr() })
    }

    pub fn head_mut(&mut self) -> Option<&mut T> {
        self.head.map(|head| unsafe { &mut *head.as_ptr() })
    }

    pub fn tail(&self) -> Option<&T> {
        self.tail.map(|tail| unsafe { &*tail.as_ptr() })
    }

    pub fn tail_mut(&mut self) -> Option<&mut T> {
        self.tail.map(|tail| unsafe { &mut *tail.as_ptr() })
    }

    pub fn count(&self) -> usize {
        if let Some(ref head) = self.head {
            unsafe { head.as_ref().count() }
        } else {
            0
        }
    }

    pub fn push_head(&mut self, node: Unique<T>) {
        debug_assert!(self.has_node(node.into()) == false, "this node was pushed already");
        let mut new_head = NonNull::from(node);

        unsafe { new_head.as_mut().init() };

        if let Some(mut old_head) = self.head {
            unsafe {
                new_head.as_mut().set_next(Some(old_head));
                old_head.as_mut().set_prev(Some(new_head));
            }
        } else {
            // The list has no node.
            self.tail = Some(new_head);
        }

        self.head = Some(new_head)
    }

    pub fn push_tail(&mut self, node: Unique<T>) {
        debug_assert!(self.has_node(node.into()) == false, "this node was pushed already");
        let mut new_tail = NonNull::from(node);

        unsafe { new_tail.as_mut().init() };

        if let Some(mut old_tail) = self.tail {
            unsafe {
                new_tail.as_mut().set_prev(Some(old_tail));
                old_tail.as_mut().set_next(Some(new_tail));
            }
        } else {
            // The list has no node.
            self.head = Some(new_tail);
        }

        self.tail = Some(new_tail)
    }

    pub fn pop_head(&mut self) -> Option<Unique<T>> {
        if let Some(mut head) = self.head {
            if let Some(mut next) = unsafe { head.as_mut().next() } {
                unsafe { next.as_mut().set_prev(None) };
                self.head = Some(next);
            } else {
                // This list has one node only.
                self.head = None;
                self.tail = None;
            }

            Some(Unique::from(head))
        } else {
            None
        }
    }

    pub fn pop_tail(&mut self) -> Option<Unique<T>> {
        if let Some(mut tail) = self.tail {
            if let Some(mut prev) = unsafe { tail.as_mut().prev() } {
                unsafe { prev.as_mut().set_next(None) };
                self.tail = Some(prev);
            } else {
                // This list has one node only.
                self.head = None;
                self.tail = None;
            }

            Some(Unique::from(tail))
        } else {
            None
        }
    }

    fn has_node(&self, target: NonNull<T>) -> bool {
        self.iter().any(|obj| ptr::eq(obj as *const _, target.as_ptr()))
    }

    pub fn detach(&mut self, mut target: NonNull<T>) {
        debug_assert!(self.has_node(target), "this node does not belong this list");

        if let Some(head) = self.head {
            if ptr::eq(head.as_ptr(), target.as_ptr()) {
                // The target is the head.
                self.pop_head();
                unsafe { target.as_mut().init() };
            } else {
                if let Some(tail) = self.tail {
                    if ptr::eq(tail.as_ptr(), target.as_ptr()) {
                        // The target is the head.
                        self.pop_tail();
                        unsafe { target.as_mut().init() };
                    }
                } else {
                    unsafe {
                        target.as_mut().detach();
                    }
                }
            }
        }
    }

    pub fn iter(&self) -> Iter<T> {
        Iter(self.head())
    }

    pub fn iter_mut(&mut self) -> IterMut<T> {
        IterMut(self.head, PhantomData)
    }
}

pub struct Iter<'a, T: 'a + Node<T>>(Option<&'a T>);

impl<'a, T: Node<T>> Iterator for Iter<'a, T> {
    type Item = &'a T;

    fn next(&mut self) -> Option<Self::Item> {
        if let Some(current) = self.0 {
            self.0 = current.next().map(|ptr| unsafe { &*ptr.as_ptr() });
            Some(current)
        } else {
            None
        }
    }
}

pub struct IterMut<'a, T: 'a + Node<T>>(Option<NonNull<T>>, PhantomData<&'a T>);

impl<'a, T: Node<T>> Iterator for IterMut<'a, T> {
    type Item = &'a mut T;

    fn next(&mut self) -> Option<Self::Item> {
        if let Some(mut ptr) = self.0 {
            unsafe {
                self.0 = ptr.as_mut().next();
                Some(&mut *ptr.as_ptr())
            }
        } else {
            None
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::alloc::{Alloc, Layout, System};
    use std::mem;

    #[derive(Debug)]
    struct Object {
        next: Option<NonNull<Object>>,
        prev: Option<NonNull<Object>>,
        order: usize,
        is_used: bool,
        hoge: usize,
        huga: usize,
    }

    // TODO: use macro or custom derive.
    impl Node<Object> for Object {
        fn set_next(&mut self, ptr: Option<NonNull<Self>>) {
            self.next = ptr;
        }

        fn set_prev(&mut self, ptr: Option<NonNull<Self>>) {
            self.prev = ptr;
        }

        fn next(&self) -> Option<NonNull<Self>> {
            self.next
        }

        fn prev(&self) -> Option<NonNull<Self>> {
            self.prev
        }
    }

    fn allocate_nodes<T>(count: usize) -> *mut T {
        let type_size = mem::size_of::<T>();
        let align = mem::align_of::<T>();
        let layout = Layout::from_size_align(count * type_size, align).unwrap();
        let ptr = unsafe { System.alloc(layout) }.unwrap();

        ptr.as_ptr() as *mut _
    }

    fn uniqued<T>(nodes: *mut T, index: usize) -> Unique<T> {
        unsafe { Unique::new_unchecked(nodes.offset(index as isize)) }
    }

    #[test]
    fn test_push_head() {
        let mut list1 = LinkedList::<Object>::new();

        assert_eq!(false, list1.head().is_some());

        const SIZE: usize = 8;
        let nodes = allocate_nodes::<Object>(SIZE);

        list1.push_head(uniqued(nodes, 0));

        assert_eq!(true, list1.head().is_some());
        if let Some(head) = list1.head_mut() {
            head.hoge = 10;
            assert_eq!(10, unsafe { (*nodes.offset(0)).hoge });
        } else {
            panic!("error");
        }
    }

    #[test]
    fn test_push_heads() {
        let mut list1 = LinkedList::<Object>::new();

        assert_eq!(false, list1.head().is_some());

        const SIZE: usize = 8;
        let nodes = allocate_nodes::<Object>(SIZE);

        list1.push_head(uniqued(nodes, 0));
        list1.push_head(uniqued(nodes, 1));
        list1.push_head(uniqued(nodes, 2));

        assert_eq!(3, list1.count());
    }

    #[test]
    fn test_push_tails() {
        let mut list1 = LinkedList::<Object>::new();

        assert_eq!(0, list1.count());

        const SIZE: usize = 8;
        let nodes = allocate_nodes::<Object>(SIZE);

        for i in 0..SIZE {
            list1.push_tail(uniqued(nodes, i));
        }
        assert_eq!(SIZE, list1.count());
    }

    #[test]
    fn test_mix_push() {
        let mut list1 = LinkedList::<Object>::new();

        assert_eq!(0, list1.count());

        const SIZE: usize = 8;
        let nodes = allocate_nodes::<Object>(SIZE);

        for i in 0..SIZE {
            if i % 2 == 0 {
                list1.push_head(uniqued(nodes, i));
            } else {
                list1.push_tail(uniqued(nodes, i));
            }
        }
        assert_eq!(SIZE, list1.count());
    }

    #[test]
    fn test_pop_heads() {
        let mut list1 = LinkedList::<Object>::new();

        const SIZE: usize = 8;
        let nodes = allocate_nodes::<Object>(SIZE);

        list1.push_head(uniqued(nodes, 0));
        list1.head_mut().unwrap().hoge = 0;
        list1.push_head(uniqued(nodes, 1));
        list1.head_mut().unwrap().hoge = 1;
        list1.push_head(uniqued(nodes, 2));
        list1.head_mut().unwrap().hoge = 2;

        assert_eq!(3, list1.count());

        for i in (0..3).rev() {
            if let Some(n) = list1.pop_head() {
                assert_eq!(i, unsafe { n.as_ref().hoge })
            } else {
                panic!("error")
            }
        }

        assert_eq!(0, list1.count());
    }

    #[test]
    fn test_pop_tails() {
        let mut list1 = LinkedList::<Object>::new();

        const SIZE: usize = 8;
        let nodes = allocate_nodes::<Object>(SIZE);

        list1.push_head(uniqued(nodes, 0));
        list1.head_mut().unwrap().hoge = 0;
        list1.push_head(uniqued(nodes, 1));
        list1.head_mut().unwrap().hoge = 1;
        list1.push_head(uniqued(nodes, 2));
        list1.head_mut().unwrap().hoge = 2;

        assert_eq!(3, list1.count());

        for i in 0..3 {
            if let Some(n) = list1.pop_tail() {
                assert_eq!(i, unsafe { n.as_ref().hoge })
            } else {
                panic!("error")
            }
        }

        assert_eq!(0, list1.count());
    }

    #[test]
    fn test_detach() {
        let mut list1 = LinkedList::<Object>::new();

        const SIZE: usize = 8;
        let nodes = allocate_nodes::<Object>(SIZE);

        for i in 0..SIZE {
            list1.push_head(uniqued(nodes, i));
            list1.head_mut().unwrap().hoge = i;
        }
        assert_eq!(SIZE, list1.count());

        // Detach the head.
        let target1 = unsafe { NonNull::new_unchecked(nodes.offset((SIZE - 1) as isize)) };
        list1.detach(target1);
        assert_eq!(SIZE - 1, list1.count());

        // Detach the tail.
        let target1 = unsafe { NonNull::new_unchecked(nodes.offset(0)) };
        list1.detach(target1);
        assert_eq!(SIZE - 2, list1.count());

        // Detach the tail.
        let target1 = unsafe { NonNull::new_unchecked(nodes.offset((SIZE / 2) as isize)) };
        list1.detach(target1);
        assert_eq!(SIZE - 2, list1.count());
    }

    #[test]
    fn test_iterator() {
        let mut list1 = LinkedList::<Object>::new();

        const SIZE: usize = 8;
        let nodes = allocate_nodes::<Object>(SIZE);

        for i in 0..SIZE {
            list1.push_head(uniqued(nodes, i));
            list1.head_mut().unwrap().hoge = i;
        }
        assert_eq!(SIZE, list1.count());

        let mut cnt = SIZE;
        for ptr in list1.iter_mut() {
            cnt -= 1;
            assert_eq!(cnt, ptr.hoge);
            ptr.hoge *= 2;
        }

        let mut cnt = SIZE;
        for ptr in list1.iter() {
            cnt -= 1;
            assert_eq!(cnt * 2, ptr.hoge);
        }
        assert_eq!(SIZE, list1.count());
    }

    #[test]
    fn test_move_nodes() {
        let mut list1 = LinkedList::<Object>::new();
        let mut list2 = LinkedList::<Object>::new();

        const SIZE: usize = 32;
        let nodes = allocate_nodes::<Object>(SIZE);

        for i in 0..SIZE {
            list1.push_head(uniqued(nodes, i));
            list1.head_mut().unwrap().hoge = i;
        }

        assert_eq!(SIZE, list1.count());
        assert_eq!(0, list2.count());
        loop {
            if let Some(node) = list1.pop_head() {
                list2.push_head(node.into());
            } else {
                break;
            }
        }
        assert_eq!(0, list1.count());
        assert_eq!(SIZE, list2.count());
    }
}
