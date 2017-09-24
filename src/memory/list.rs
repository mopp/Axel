#![cfg_attr(test, feature(allocator_api))]

use core::convert::{AsRef, AsMut};
use core::default::Default;
use core::ptr::{Unique, Shared};



/// LinkedList struct.
pub struct LinkedList<T: Default> {
    // There two fields are just dummy node to implement Node::detach easily.
    head: Node<T>,
    tail: Node<T>,
}


pub struct Node<T: Default> {
    next: Option<Shared<Node<T>>>,
    prev: Option<Shared<Node<T>>>,
    element: T,
}


impl<T: Default> LinkedList<T> {
    pub fn new() -> LinkedList<T>
    {
        LinkedList {
            head: Default::default(),
            tail: Default::default(),
        }
    }

    pub fn len(&self) -> usize
    {
        let mut node =
            if let Some(ref head) = self.head.next {
                unsafe { head.as_ref() }
            } else  {
                return 0;
            };

        let mut cnt = 1;
        loop {
            match node.next {
                None => break,
                Some(ref next) => {
                    node = unsafe {next.as_ref()};
                    cnt += 1;
                }
            }
        }
        cnt
    }

    pub fn front(&self) -> Option<&T>
    {
        unsafe {
            self.head.next.as_ref().map(|node| &node.as_ref().element)
        }
    }

    pub fn front_mut(&mut self) -> Option<&mut T>
    {
        unsafe {
            self.head.next.as_mut().map(|node| &mut node.as_mut().element)
        }
    }

    pub fn back(&self) -> Option<&T>
    {
        unsafe {
            self.tail.prev.as_ref().map(|node| &node.as_ref().element)
        }
    }

    pub fn back_mut(&mut self) -> Option<&mut T>
    {
        unsafe {
            self.tail.prev.as_mut().map(|node| &mut node.as_mut().element)
        }
    }

    pub fn push_front(&mut self, new_node: Unique<Node<T>>)
    {
        let mut new_shared_node = Shared::from(new_node);

        {
            let n = unsafe { new_shared_node.as_mut() };
            n.next = self.head.next;
            n.prev = None;
        }

        let new_shared_node = Some(new_shared_node);
        match self.head.next {
            None           => self.tail.prev = new_shared_node,
            Some(mut head) => unsafe { head.as_mut().prev = new_shared_node },
        }

        self.head.next = new_shared_node;
    }

    pub fn push_back(&mut self, new_node: Unique<Node<T>>)
    {
        let mut new_shared_node = Shared::from(new_node);

        {
            let n = unsafe { new_shared_node.as_mut() };
            n.next = None;
            n.prev = self.tail.prev;
        }

        let new_shared_node = Some(new_shared_node);
        match self.tail.prev {
            None  => self.head.next = new_shared_node,
            Some(mut tail) => unsafe {tail.as_mut().next = new_shared_node},
        }

        self.tail.prev = new_shared_node;
    }

    pub fn pop_front(&mut self) -> Option<Unique<Node<T>>>
    {
        match self.head.next {
            None       => None,
            Some(head) => {
                self.head.next = unsafe { head.as_ref().next };

                match self.head.next {
                    None               => self.tail.prev = None,
                    Some(mut new_head) => unsafe { new_head.as_mut().prev = None },
                }

                unsafe { Some(Unique::new_unchecked(head.as_ptr())) }
            }
        }
    }

    pub fn pop_back(&mut self) -> Option<Unique<Node<T>>>
    {
        match self.tail.prev {
            None       => None,
            Some(tail) => {
                self.tail.prev = unsafe { tail.as_ref().prev };

                match self.tail.prev {
                    None               => self.head.next = None,
                    Some(mut new_tail) => unsafe { new_tail.as_mut().next = None },
                }

                unsafe { Some(Unique::new_unchecked(tail.as_ptr())) }
            }
        }
    }
}


impl<T: Default> Default for Node<T> {
    fn default() -> Node<T>
    {
        Node {
            next: None,
            prev: None,
            element: Default::default(),
        }
    }
}


impl<T: Default> Node<T> {
    pub fn detach(&mut self)
    {
        if let Some(mut next) = self.next {
            let next = unsafe { next.as_mut() };
            next.prev = self.prev;
        }

        if let Some(mut prev) = self.prev {
            let prev = unsafe { prev.as_mut() };
            prev.next = self.next;
        }

        self.next = None;
        self.prev = None;
    }
}


impl<T: Default> AsRef<T> for Node<T> {
    fn as_ref(&self) -> &T
    {
        &self.element
    }
}


impl<T: Default> AsMut<T> for Node<T> {
    fn as_mut(&mut self) -> &mut T
    {
        &mut self.element
    }
}


#[cfg(test)]
mod tests {
    use super::{LinkedList, Node};

    use std::heap::{Alloc, System, Layout};
    use std::mem;
    use std::slice;
    use std::ptr::Unique;

    fn allocate_node_objs<'a, T>(count: usize) -> &'a mut [T] where T: Default
    {
        let type_size = mem::size_of::<T>();
        let align     = mem::align_of::<T>();
        let layout    = Layout::from_size_align(count * type_size, align).unwrap();
        let ptr       = unsafe { System.alloc(layout) }.unwrap();
        unsafe { slice::from_raw_parts_mut(ptr as *mut T, count) }
    }

    #[test]
    fn test_push_front()
    {
        let objs = allocate_node_objs::<Node<usize>>(1024);

        let mut list = LinkedList::new();
        list.push_front(unsafe {Unique::new_unchecked(&mut objs[0])});
        assert_eq!(list.len(), 1);
        assert_eq!(list.back(), Some(&0usize));
        assert_eq!(list.front(), Some(&0usize));
    }

    #[test]
    fn test_push_back()
    {
        let objs = allocate_node_objs::<Node<usize>>(1024);

        let mut list = LinkedList::new();
        list.push_back(unsafe {Unique::new_unchecked(&mut objs[1])});
        assert_eq!(list.len(), 1);
        assert_eq!(list.back(), Some(&0usize));
        assert_eq!(list.front(), Some(&0usize));
    }

    #[test]
    fn test_pop_front()
    {
        let objs = allocate_node_objs::<Node<usize>>(128);

        let mut list = LinkedList::new();
        for (i, o) in objs.iter_mut().enumerate() {
            o.element = i;

            list.push_front(unsafe {Unique::new_unchecked(o)});
        }

        assert_eq!(list.len(), objs.len());
        assert_eq!(list.back(), Some(&0usize));
        assert_eq!(list.front(), Some(&(objs.len() - 1)));

        for i in (0..objs.len()).rev() {
            match list.pop_front() {
                None       => panic!("error"),
                Some(node) => {
                    assert_eq!(i, unsafe {node.as_ref().element});
                }
            }
        }
    }

    #[test]
    fn test_accessors()
    {
        let objs = allocate_node_objs::<Node<usize>>(128);

        let mut list = LinkedList::new();
        for (i, o) in objs.iter_mut().enumerate() {
            o.element = i;

            list.push_front(unsafe {Unique::new_unchecked(o)});
        }

        assert_eq!(list.len(), objs.len());
        assert_eq!(list.back_mut(), Some(&mut 0));
        assert_eq!(list.pop_back().is_some(), true);

        *list.front_mut().unwrap() = 10;
        assert_eq!(list.front(), Some(&10));
        match list.pop_front() {
            None  => panic!("error"),
            Some(node) => {
                assert_eq!(unsafe {node.as_ref().element}, 10);
            }
        }
        assert_eq!(list.len(), objs.len() - 2);

        objs[1].detach();
        assert_eq!(list.len(), objs.len() - 3);
        assert_eq!(list.head.next.unwrap().as_ptr(), &mut objs[128 - 2] as *mut _);
        assert_eq!(list.head.prev.is_none(), true);
        assert_eq!(list.tail.next.is_none(), true);
        assert_eq!(list.tail.prev.unwrap().as_ptr(), &mut objs[1] as *mut _);

        *(objs[0].as_mut()) = 10;
        assert_eq!(objs[0].as_ref(), &10);
    }
}
