extern crate core;
use core::ptr::Shared;


type Link<T> = Option<Shared<AList<T>>>;


pub struct AList<T>  {
    front: Link<T>,
    back: Link<T>,
    content: Shared<T>
}


// Static methods.
impl<T> AList<T> {
    #[allow(dead_code)]
    fn new(content: &mut T) -> AList<T>
    {
        AList {
            front: None,
            back: None,
            content: unsafe {Shared::new(content)},
        }
    }

    #[allow(dead_code)]
    fn make_link(node: &mut AList<T>) -> Link<T>
    {
        Some(unsafe {Shared::new(node)})
    }


    #[allow(dead_code)]
    fn link<'a>(l: Link<T>) -> Option<&'a AList<T>>
    {
        match l {
            None => None,
            Some(c) => Some(unsafe {&**c}),
        }
    }


    #[allow(dead_code)]
    fn link_mut<'a>(l: Link<T>) -> Option<&'a mut AList<T>>
    {
        match l {
            None => None,
            Some(c) => Some(unsafe {&mut **c}),
        }
    }

    #[allow(dead_code)]
    fn link_each_other(node1: &mut AList<T>, node2: &mut AList<T>)
    {
        let node1_opt = AList::make_link(node1);
        let node2_opt = AList::make_link(node2);

        node1.front = node2_opt;
        node1.back  = node2_opt;
        node2.front = node1_opt;
        node2.back  = node1_opt;
    }


    #[allow(dead_code)]
    fn push_guard(pushed: &AList<T>, new: &AList<T>) {
        if new.is_empty() == false {
            panic!("New node should NOT be linked to other nodes.");
        }

        if new == pushed {
            panic!("We cannot push self into self.");
        }
    }
}

impl<T> AList<T> {
    #[allow(dead_code)]
    pub fn init(&mut self, content: &mut T)
    {
        self.front   = None;
        self.back    = None;
        self.content = unsafe {Shared::new(content)};
    }


    #[allow(dead_code)]
    pub fn iter(&self) -> Iter<T>
    {
        Iter {
            begin_node: self,
            current_node: self,
            is_first: true,
        }
    }


    #[allow(dead_code)]
    pub fn iter_mut(&self) -> IterMut<T>
    {
        IterMut {
            begin_node: self,
            current_node: self,
            is_first: true,
        }
    }


    #[allow(dead_code)]
    pub fn is_empty(&self) -> bool
    {
        match (self.front, self.back) {
            (None, None) => true,
            _            => false,
        }
    }


    #[allow(dead_code)]
    pub fn front(&self) -> Option<&AList<T>>
    {
        AList::link(self.front)
    }


    #[allow(dead_code)]
    pub fn front_mut(&self) -> Option<&mut AList<T>>
    {
        AList::link_mut(self.front)
    }


    #[allow(dead_code)]
    pub fn back(&self) -> Option<&AList<T>>
    {
        AList::link(self.back)
    }


    #[allow(dead_code)]
    pub fn back_mut(&self) -> Option<&mut AList<T>>
    {
        AList::link_mut(self.back)
    }


    #[allow(dead_code)]
    pub fn push_front<'b>(&mut self, new: &'b mut AList<T>)
    {
        AList::push_guard(self, new);

        if self.is_empty() == true {
            // If self is not linked by any other nodes.
            AList::link_each_other(self, new);
            return;
        }

        let self_opt = AList::make_link(self);
        new.front = self.front;
        new.back = self_opt;

        let new_opt = AList::make_link(new);
        self.front_mut().unwrap().back = new_opt;
        self.front = new_opt;
    }


    #[allow(dead_code)]
    pub fn pop_front(&mut self) -> Option<Shared<T>>
    {
        match self.front {
            None => None,
            Some(shared_front) => {
                let front  = unsafe { &**shared_front };
                self.front = front.front;
                let tmp    = unsafe { &mut **self.front.unwrap()};
                tmp.back   = AList::make_link(self);

                Some(front.content)
            }
        }
    }


    #[allow(dead_code)]
    pub fn push_back(&mut self, new: &mut AList<T>)
    {
        AList::push_guard(self, new);

        if self.is_empty() == true {
            // If self is not linked by any other nodes.
            AList::link_each_other(self, new);
            return;
        }

        let self_opt = AList::make_link(self);
        new.front = self_opt;
        new.back  = self.back;

        let new_opt = AList::make_link(new);
        self.back_mut().unwrap().front = new_opt;
        self.back = new_opt;
    }


    #[allow(dead_code)]
    pub fn pop_back(&mut self) -> Option<Shared<T>>
    {
        match self.back {
            None => None,
            Some(shared_back) => {
                let back  = unsafe { &**shared_back };
                self.back = back.back;
                let tmp   = unsafe { &mut **self.back.unwrap()};
                tmp.front = AList::make_link(self);

                Some(back.content)
            }
        }
    }


    #[allow(dead_code)]
    pub fn borrow(&self) -> &T
    {
        unsafe {
            &**self.content
        }
    }


    #[allow(dead_code)]
    pub fn borrow_mut(&self) -> &mut T
    {
        unsafe{&mut **self.content}
    }
}


impl<T> PartialEq for AList<T> {
    fn eq(&self, other: &AList<T>) -> bool
    {
        let addr_self  = (self as *const _) as usize;
        let addr_other = (other as *const _) as usize;
        addr_self == addr_other
    }
}


macro_rules! def_iter_struct {
    ($i:ident) => {
        pub struct $i<'a, T: 'a> {
            begin_node: &'a AList<T>,
            current_node: &'a AList<T>,
            is_first: bool,
        }
    };
}


def_iter_struct!(Iter);


impl<'a, T> Iterator for Iter<'a, T> {
    type Item = &'a T;

    fn next(&mut self) -> Option<Self::Item>
    {
        let old_current_node = self.current_node;
        let value            = old_current_node.borrow();
        self.current_node    = old_current_node.front().unwrap();

        if self.is_first {
            self.is_first = false;
            return Some(value);
        }

        if self.begin_node == old_current_node {
            return None;
        }

        Some(value)
    }
}


def_iter_struct!(IterMut);


impl<'a, T> Iterator for IterMut<'a, T> {
    type Item = &'a mut T;

    fn next(&mut self) -> Option<Self::Item>
    {
        let old_current_node = self.current_node;
        let value            = old_current_node.borrow_mut();
        self.current_node    = old_current_node.front().unwrap();

        if self.is_first {
            self.is_first = false;
            return Some(value);
        }

        // println!("{:x} vs {:x}", (self.begin_node  as *const _) as usize, (old_current_node  as *const _) as usize);
        if self.begin_node == old_current_node {
            return None;
        }

        Some(value)
    }
}


#[cfg(test)]
mod test {
    use super::AList;
    use core::mem;

    #[test]
    fn test_push_front()
    {
        let mut n1 = AList::<usize>::new(&mut 0);
        let mut n2 = AList::<usize>::new(&mut 1);

        n1.push_front(&mut n2);

        assert_eq!(0, *n1.borrow());
        assert_eq!(1, *n1.front().unwrap().borrow());
        assert_eq!(1, *n1.back().unwrap().borrow());
        {
            let n2 = n1.back().unwrap();
            assert_eq!(0, *n2.front().unwrap().borrow());
            assert_eq!(0, *n2.back().unwrap().borrow());
        }

        *n1.borrow_mut() = 7;
        {
            let n2 = n1.front().unwrap();
            assert_eq!(7, *n2.front().unwrap().borrow());
            assert_eq!(7, *n2.back().unwrap().borrow());
        }

        let mut n3 = AList::<usize>::new(&mut 2);
        n1.push_front(&mut n3);
        {
            let n2 = n1.back().unwrap();
            assert_eq!(1, *n2.borrow());
            assert_eq!(2, *n2.back().unwrap().borrow());
            assert_eq!(7, *n2.back().unwrap().back().unwrap().borrow());
        }

        {
            let n3 = n1.front().unwrap();
            assert_eq!(2, *n3.borrow());
            assert_eq!(1, *n3.front().unwrap().borrow());
            assert_eq!(7, *n3.front().unwrap().front().unwrap().borrow());
        }

        let mut n4 = AList::<usize>::new(&mut 3);
        n1.push_front(&mut n4);
        {
            let n4 = n1.front().unwrap();
            assert_eq!(3, *n4.borrow());
            assert_eq!(2, *n4.front().unwrap().borrow());
            assert_eq!(1, *n4.front().unwrap().front().unwrap().borrow());
            assert_eq!(7, *n4.back().unwrap().borrow());
        }

        let mut n1 = AList::new(&mut 0);
        let mut n2 = AList::new(&mut 1);
        let mut n3 = AList::new(&mut 2);
        let mut n4 = AList::new(&mut 3);
        n1.push_front(&mut n2);
        n1.push_front(&mut n3);
        n1.push_front(&mut n4);
        assert_eq!(0, *n1.borrow());
        assert_eq!(3, *n1.front().unwrap().borrow());
        assert_eq!(2, *n1.front().unwrap().front().unwrap().borrow());
        assert_eq!(1, *n1.back().unwrap().borrow());
    }


    #[test]
    fn test_push_back()
    {
        let mut n1 = AList::<usize>::new(&mut 0);
        let mut n2 = AList::<usize>::new(&mut 1);

        n1.push_back(&mut n2);

        assert_eq!(1, *n1.front().unwrap().borrow());
        assert_eq!(1, *n1.back().unwrap().borrow());
        {
            let n2 = n1.back().unwrap();
            assert_eq!(0, *n2.front().unwrap().borrow());
            assert_eq!(0, *n2.back().unwrap().borrow());
        }

        *n1.borrow_mut() = 7;
        {
            let n2 = n1.front().unwrap();
            assert_eq!(7, *n2.front().unwrap().borrow());
            assert_eq!(7, *n2.back().unwrap().borrow());
        }

        let mut n3 = AList::<usize>::new(&mut 2);
        n1.push_back(&mut n3);
        {
            let n3 = n1.back().unwrap();
            assert_eq!(2, *n3.borrow());
            assert_eq!(1, *n3.back().unwrap().borrow());
            assert_eq!(7, *n3.back().unwrap().back().unwrap().borrow());
        }

        {
            let n2 = n1.front().unwrap();
            assert_eq!(1, *n2.borrow());
            assert_eq!(2, *n2.front().unwrap().borrow());
            assert_eq!(7, *n2.front().unwrap().front().unwrap().borrow());
        }

        let mut n4 = AList::<usize>::new(&mut 3);
        n1.push_back(&mut n4);
        {
            let n4 = n1.back().unwrap();
            assert_eq!(3, *n4.borrow());
            assert_eq!(7, *n4.front().unwrap().borrow());
            assert_eq!(1, *n4.front().unwrap().front().unwrap().borrow());
            assert_eq!(2, *n4.back().unwrap().borrow());
        }
    }


    #[test]
    fn test_itr()
    {
        let mut n1 = AList::new(&mut 0);
        let mut n2 = AList::new(&mut 1);
        let mut n3 = AList::new(&mut 2);
        let mut n4 = AList::new(&mut 3);

        n1.push_front(&mut n2);
        n1.front_mut().unwrap().push_front(&mut n3);
        n1.front_mut().unwrap().front_mut().unwrap().push_front(&mut n4);

        let mut itr = n1.iter();
        assert_eq!(0, *itr.next().unwrap());
        assert_eq!(1, *itr.next().unwrap());
        assert_eq!(2, *itr.next().unwrap());
        assert_eq!(3, *itr.next().unwrap());
        assert_eq!(None, itr.next());

        let mut cnt = 0;
        let ans = [0, 1, 2, 3];
        for i in n1.iter() {
            assert_eq!(ans[cnt], *i);
            cnt += 1;
        }
    }


    #[test]
    fn test_itr_mut()
    {
        let mut n1 = AList::new(&mut 0);
        let mut n2 = AList::new(&mut 1);
        let mut n3 = AList::new(&mut 2);
        let mut n4 = AList::new(&mut 3);

        n1.push_front(&mut n2);
        n1.front_mut().unwrap().push_front(&mut n3);
        n1.front_mut().unwrap().front_mut().unwrap().push_front(&mut n4);

        let mut cnt = 0;
        let ans = [0, 1, 2, 3];
        for i in n1.iter_mut() {
            assert_eq!(ans[cnt], *i);
            cnt += 1;

            *i += 999;
        }

        cnt = 0;
        for i in n1.iter() {
            assert_eq!(ans[cnt] + 999, *i);
            cnt += 1;
        }
    }


    #[test]
    fn test_pop()
    {
        let mut n1 = AList::new(&mut 0);
        let mut n2 = AList::new(&mut 1);
        let mut n3 = AList::new(&mut 2);
        let mut n4 = AList::new(&mut 3);

        n1.push_front(&mut n2);
        n1.front_mut().unwrap().push_front(&mut n3);
        n1.front_mut().unwrap().front_mut().unwrap().push_front(&mut n4);

        let mut cnt = 0;
        let ans = [0, 1, 2, 3];
        for i in n1.iter() {
            assert_eq!(ans[cnt], *i);
            cnt += 1;
        }

        let content = unsafe {&**n1.pop_front().unwrap()};
        assert_eq!(1, *content);

        let mut cnt = 0;
        let ans = [0, 2, 3];
        for i in n1.iter() {
            assert_eq!(ans[cnt], *i);
            cnt += 1;
        }

        let content = unsafe { &**n1.pop_back().unwrap() };
        assert_eq!(3, *content);
        let mut cnt = 0;
        let ans = [0, 2];
        for i in n1.iter() {
            assert_eq!(ans[cnt], *i);
            cnt += 1;
        }
    }

    #[test]
    fn test_array()
    {
        let mut n1 = AList::new(&mut 0);
        let mut arr: [usize; 3] = [1, 2, 3];
        let mut src: [AList<usize>; 3];

        unsafe {
            src = mem::uninitialized();
        }

        for i in 0..arr.len() {
            src[i].init(&mut arr[i]);
            n1.push_front(&mut src[i]);
        }

        let mut cnt = 0;
        let ans = [0, 3, 2, 1];
        for i in n1.iter() {
            assert_eq!(ans[cnt], *i);
            cnt += 1;
        }
    }
}
