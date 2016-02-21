#![allow(dead_code)]
use core::ptr::Shared;
use core::marker;


type Link<T> = Option<Shared<Node<T>>>;


pub trait Get {
    type T;
    unsafe fn get<'a>(&self) -> Option<&'a Self::T>;
    unsafe fn get_mut<'a>(&self) -> Option<&'a mut Self::T>;
}


impl<T> Get for Link<T> {
    type T = Node<T>;

    unsafe fn get<'a>(&self) -> Option<&'a Self::T>
    {
        match *self {
            None => None,
            Some(shared) => Some(&**shared),
        }
    }


    unsafe fn get_mut<'a>(&self) -> Option<&'a mut Self::T>
    {
        match *self {
            None => None,
            Some(shared) => Some(&mut**shared),
        }
    }
}


pub struct Node<T> {
    next: Link<T>,
    prev: Link<T>,
    value: T,
}


impl<T> Node<T> {
    fn new(v: T) -> Node<T>
    {
        Node {
            next: None,
            prev: None,
            value: v,
        }
    }


    pub fn init(&mut self, v: T)
    {
        self.next  = None;
        self.prev  = None;
        self.value = v;
    }


    fn make_link(n: &mut Node<T>) -> Link<T>
    {
        Some(unsafe {Shared::new(n)})
    }


    fn set_next(&mut self, next: &mut Node<T>)
    {
        self.next = Node::make_link(next);
        next.prev = Node::make_link(self);
    }


    pub fn get(&self) -> &T
    {
        &self.value
    }


    pub fn get_mut(&mut self) -> &mut T
    {
        &mut self.value
    }


    pub fn disjoint(&mut self, belong_list: &mut AList<T>)
    {
        match (self.next, self.prev) {
            (None, None) => {},
            (Some(next_shared), None) => {
                // This node is a head of the list.
                let next_node    = unsafe { &mut **next_shared };
                next_node.prev   = None;
                belong_list.head = self.next;
            },
            (None, Some(prev_shared)) => {
                // This node is a tail of the list.
                let prev_node    = unsafe { &mut **prev_shared };
                prev_node.next   = None;
                belong_list.tail = self.prev;
            },
            (Some(next_shared), Some(prev_shared)) => {
                let next_node = unsafe { &mut **next_shared };
                let prev_node = unsafe { &mut **prev_shared };
                prev_node.set_next(next_node);
            },
        };
    }
}


pub struct AList<T> {
    head: Link<T>,
    tail: Link<T>,
}


macro_rules! gen_accessor {
    ($func_name: ident, $var: ident, $getter: ident, $ret_type:ty) => {
        fn $func_name(&self) -> Option<$ret_type>
        {
            match unsafe { self.$var.$getter() } {
                None => None,
                Some(node) => Some(node.$getter()),
            }
        }
    };
}


impl<T> AList<T> {
    fn new() -> AList<T>
    {
        AList {
            head: None,
            tail: None,
        }
    }


    fn iter(&self) -> Iter<T>
    {
        Iter {
            link: self.head,
            _marker: marker::PhantomData,
        }
    }


    fn iter_mut(&self) -> IterMut<T>
    {
        IterMut {
            link: self.head,
            _marker: marker::PhantomData,
        }
    }


    pub fn is_empty(&self) -> bool
    {
        self.head.is_none() && self.tail.is_none()
    }


    fn len(&self) -> usize
    {
        let mut cnt = 0;
        for _ in self.iter() {
            cnt += 1;
        }
        cnt
    }


    fn reset_links(&mut self, link: Link<T>)
    {
        self.head = link;
        self.tail = link;
    }


    gen_accessor!(front, head, get, &T);
    gen_accessor!(front_mut, head, get_mut, &mut T);
    gen_accessor!(back, tail, get, &T);
    gen_accessor!(back_mut, tail, get_mut, &mut T);


    pub fn push_front<'a>(&mut self, new: &'a mut Node<T>)
    {
        match self.head {
            None => {
                self.reset_links(Node::make_link(new));
            }
            Some(shared) => {
                let node = unsafe { &mut **shared };
                new.set_next(node);
                self.head = Node::make_link(new);
            }
        }
    }


    pub fn pop_front<'a>(&mut self) -> Option<&'a mut Node<T>>
    {
        match self.head {
            None => None,
            Some(shared) => {
                let node = unsafe { &mut **shared };
                self.head = node.next;
                match node.next {
                    None => {
                        self.reset_links(None);
                    },
                    Some(next_shared) => {
                        let next_node = unsafe { &mut **next_shared };
                        next_node.prev = None;
                    }
                }
                Some(node)
            },
        }
    }


    pub fn push_back<'a>(&mut self, new: &'a mut Node<T>)
    {
        match self.tail {
            None => {
                self.reset_links(Node::make_link(new));
            }
            Some(shared) => {
                let node = unsafe { &mut **shared };
                node.set_next(new);
                self.tail = Node::make_link(new);
            }
        }
    }


    fn pop_back<'a>(&mut self) -> Option<&'a mut Node<T>>
    {
        match self.tail {
            None => None,
            Some(shared) => {
                let node = unsafe { &mut **shared };
                self.tail = node.prev;
                match node.prev {
                    None => {
                        self.reset_links(None);
                    },
                    Some(prev_shared) => {
                        let prev_node = unsafe { &mut **prev_shared };
                        prev_node.next = None;
                    }
                }
                Some(node)
            },
        }
    }
}


struct Iter<'a, T: 'a> {
    link: Link<T>,
    _marker: marker::PhantomData<&'a T>,
}


impl<'a, T> Iterator for Iter<'a, T> {
    type Item = &'a T;

    fn next(&mut self) -> Option<Self::Item>
    {
        match self.link {
            None => None,
            Some(_) => {
                let current_node  = unsafe { self.link.get() }.unwrap();
                self.link         = current_node.next;
                Some(current_node.get())
            },
        }
    }
}


struct IterMut<'a, T: 'a> {
    link: Link<T>,
    _marker: marker::PhantomData<&'a T>,
}


impl<'a, T> Iterator for IterMut<'a, T> {
    type Item = &'a T;

    fn next(&mut self) -> Option<Self::Item>
    {
        match self.link {
            None => None,
            Some(_) => {
                let current_node  = unsafe { self.link.get_mut() }.unwrap();
                self.link         = current_node.next;
                Some(current_node.get())
            },
        }
    }
}


#[cfg(test)]
mod test {
    use core::mem;
    use Node;
    use AList;
    use Get;

    macro_rules! addr_of_var {
        ($x: expr) => (address_of_ref!(&$x));
    }

    macro_rules! addr_of_ref {
        ($x: expr) => (($x as *const _) as usize);
    }


    #[test]
    fn test_get()
    {
        let mut n1 = Node::new(0usize);
        assert_eq!(*n1.get(), 0);
        *n1.get_mut() = 10;
        assert_eq!(*n1.get(), 10);
    }


    #[test]
    fn test_iters()
    {
        const N: usize = 100;
        let mut node_array: [Node<usize>; N] = unsafe { mem::uninitialized() };
        let node_slice = &mut node_array;
        for i in 0..node_slice.len() {
            node_slice[i].init(i);
        }

        let mut list = AList::<usize>::new();
        let mut cnt = 0;
        for node in node_slice.iter_mut() {
            assert_eq!(cnt, *node.get());
            cnt += 1;

            list.push_front(node);
        }

        assert_eq!(list.len(), N);

        cnt = N;
        for v in list.iter_mut() {
            cnt -= 1;
            assert_eq!(*v, cnt);
            assert_eq!(addr_of_ref!(v), addr_of_ref!(node_slice[cnt].get()));
        }

        cnt = N;
        for v in list.iter() {
            cnt -= 1;
            assert_eq!(*v, cnt);
            assert_eq!(addr_of_ref!(v), addr_of_ref!(node_slice[cnt].get()));
        }
    }


    #[test]
    fn test_is_empty()
    {
        let list = AList::<usize>::new();
        assert_eq!(list.is_empty(), true);
    }


    #[test]
    fn test_push_front()
    {
        let mut n1 = Node::new(0usize);
        let mut n2 = Node::new(1usize);
        let mut list = AList::<usize>::new();
        list.push_front(&mut n1);

        assert_eq!(list.is_empty(), false);
        assert_eq!(list.len(), 1);
        assert_eq!(unsafe { list.head.get().unwrap().value }, unsafe { list.tail.get().unwrap().value });

        list.push_front(&mut n2);
        assert_eq!(unsafe { list.head.get().unwrap().value }, unsafe { list.tail.get().unwrap().prev.get().unwrap().value});
    }


    #[test]
    fn test_push_back()
    {
        let mut n1 = Node::new(0usize);
        let mut n2 = Node::new(1usize);
        let mut list = AList::<usize>::new();
        list.push_back(&mut n1);

        assert_eq!(list.is_empty(), false);
        assert_eq!(list.len(), 1);
        assert_eq!(unsafe { list.head.get().unwrap().value }, unsafe { list.tail.get().unwrap().value });

        list.push_back(&mut n2);
        assert_eq!(unsafe { list.head.get().unwrap().value }, unsafe { list.tail.get().unwrap().prev.get().unwrap().value});
    }


    #[test]
    fn test_front_back()
    {
        let mut n1   = Node::new(0usize);
        let mut n2   = Node::new(1usize);
        let mut list = AList::<usize>::new();
        list.push_front(&mut n1);
        list.push_front(&mut n2);

        assert_eq!(list.front(), Some(&1));
        assert_eq!(list.back(), Some(&0));
    }


    #[test]
    fn test_front_back_mut()
    {
        let mut n1   = Node::new(0usize);
        let mut n2   = Node::new(1usize);
        let mut list = AList::<usize>::new();
        list.push_front(&mut n1);
        list.push_front(&mut n2);

        *(list.front_mut().unwrap()) = 100;
        *(list.back_mut().unwrap()) = 200;

        assert_eq!(list.front(), Some(&100));
        assert_eq!(list.back(), Some(&200));
    }


    #[test]
    fn test_pop_front()
    {
        let mut n1   = Node::new(0usize);
        let mut n2   = Node::new(1usize);
        let mut n3   = Node::new(2usize);
        let mut list = AList::<usize>::new();
        list.push_front(&mut n1);
        list.push_front(&mut n2);
        list.push_front(&mut n3);

        assert_eq!(list.len(), 3);
        assert_eq!(list.pop_front().unwrap().get(), n3.get());
        assert_eq!(list.len(), 2);
        assert_eq!(list.pop_front().unwrap().get(), n2.get());
        assert_eq!(list.len(), 1);
        assert_eq!(list.pop_back().unwrap().get(), n1.get());
        assert_eq!(list.len(), 0);
        assert_eq!(list.pop_front().is_none(), true);
    }


    #[test]
    fn test_pop_back()
    {
        let mut n1   = Node::new(0usize);
        let mut n2   = Node::new(1usize);
        let mut n3   = Node::new(2usize);
        let mut list = AList::<usize>::new();
        list.push_front(&mut n1);
        list.push_front(&mut n2);
        list.push_front(&mut n3);

        assert_eq!(list.len(), 3);
        assert_eq!(list.pop_back().unwrap().get(), n1.get());
        assert_eq!(list.len(), 2);
        assert_eq!(list.pop_back().unwrap().get(), n2.get());
        assert_eq!(list.len(), 1);
        assert_eq!(list.pop_front().unwrap().get(), n3.get());
        assert_eq!(list.len(), 0);
        assert_eq!(list.pop_back().is_none(), true);
    }

    #[test]
    fn test_disjoint()
    {
        let mut n1   = Node::new(0usize);
        let mut n2   = Node::new(1usize);
        let mut n3   = Node::new(2usize);
        let mut list = AList::<usize>::new();
        list.push_front(&mut n1);
        list.push_front(&mut n2);
        list.push_front(&mut n3);

        assert_eq!(list.len(), 3);
        n2.disjoint(&mut list);
        assert_eq!(list.len(), 2);

        let mut i = 0;
        let ans = [2, 0];
        for v in list.iter() {
            assert_eq!(*v, ans[i]);
            i += 1;
        }
    }
}
