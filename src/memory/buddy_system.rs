use core::mem;
use core::ptr;
use core::ptr::Unique;
use memory::frame::{Frame, State};
use memory::list::LinkedList;
use memory::list::Node;


// 2^MAX_ORDER
pub const MAX_ORDER: usize = 16 + 1;


pub struct BuddyManager {
    nodes: Unique<Node<Frame>>,
    count_frames: usize,
    base_addr: usize,
    frame_size_unit: usize,
    count_free_frames: [usize; MAX_ORDER],
    lists: [LinkedList<Frame>; MAX_ORDER],
}


impl BuddyManager {
    pub fn new(nodes: *mut Node<Frame>, count: usize, base_addr: usize, frame_size_unit: usize) -> BuddyManager {
        let lists = unsafe {
            let mut lists: [LinkedList<Frame>; MAX_ORDER] = mem::uninitialized();

            for l in lists.iter_mut() {
                ptr::write(l, LinkedList::new())
            }

            lists
        };

        let nodes = match Unique::new(nodes) {
            Some(u) => u,
            None => panic!("The node pointer is null !"),
        };

        let mut bman = BuddyManager {
            nodes: nodes,
            count_frames: count,
            base_addr: base_addr,
            frame_size_unit: frame_size_unit,
            lists: lists,
            count_free_frames: [0; MAX_ORDER],
        };

        bman.supply_frame_nodes(nodes, count);

        bman
    }

    fn push_node_frame(&mut self, order: usize, node_ptr: Unique<Node<Frame>>) {
        self.lists[order].push_back(node_ptr);
        self.count_free_frames[order] += 1;
    }

    fn pop_node_frame(&mut self, order: usize) -> Option<Unique<Node<Frame>>> {
        self.count_free_frames[order] -= 1;
        self.lists[order].pop_front()
    }

    fn is_empty_list(&self, order: usize) -> bool {
        self.count_free_frames[order] == 0
    }

    fn count_free_frames(&self) -> usize {
        self.count_free_frames
            .iter()
            .enumerate()
            .fold(0, |acc, (order, &x)| acc + (x * (1 << order)))
    }

    fn count_used_frames(&self) -> usize {
        self.count_frames - self.count_free_frames()
    }

    pub fn free_memory_size(&self) -> usize {
        self.count_free_frames() * self.frame_size_unit
    }

    pub fn used_memory_size(&self) -> usize {
        self.count_used_frames() * self.frame_size_unit
    }

    fn supply_frame_nodes(&mut self, nodes: Unique<Node<Frame>>, count: usize) {
        debug_assert!(count != 0);

        let mut count_rest_frames = count;
        let mut current_node_ptr = nodes.as_ptr();

        for order in (0..MAX_ORDER).rev() {
            let count_frames_in_list = 1usize << order;
            while count_frames_in_list <= count_rest_frames {
                self.push_node_frame(order, unsafe { Unique::new_unchecked(current_node_ptr) });

                current_node_ptr = unsafe { current_node_ptr.offset(count_frames_in_list as isize) };
                count_rest_frames -= count_frames_in_list;
            }
        }
    }

    fn get_frame_index(&self, node: Unique<Node<Frame>>) -> usize {
        self.nodes.as_ptr().offset_to(node.as_ptr()).unwrap() as usize
    }

    fn get_buddy_node(&mut self, node: Unique<Node<Frame>>, order: usize) -> Unique<Node<Frame>> {
        let buddy_index = self.get_frame_index(node) ^ (1 << order);
        unsafe { Unique::new_unchecked(self.nodes.as_ptr().offset(buddy_index as isize)) }
    }

    fn get_addr(&self, node: Unique<Node<Frame>>) -> usize {
        self.base_addr + self.get_frame_index(node) * self.frame_size_unit
    }

    pub fn allocate_frame_by_order(&mut self, request_order: usize) -> Option<Unique<Node<Frame>>> {
        if MAX_ORDER <= request_order {
            return None;
        }

        for order in request_order..MAX_ORDER {
            if self.is_empty_list(order) {
                continue;
            }

            let node_opt = self.pop_node_frame(order);

            match node_opt {
                None => panic!("the counter may be an error"),
                Some(mut node) => {
                    for i in (request_order..order).rev() {
                        let mut buddy_node = self.get_buddy_node(node, i);
                        {
                            let buddy_frame = unsafe { buddy_node.as_mut() }.as_mut();
                            buddy_frame.set_order(i);
                        }

                        self.push_node_frame(i, buddy_node);
                    }

                    // Set the order and the extra parts are stored into the other lists.
                    let allocated_frame = unsafe { node.as_mut() }.as_mut();
                    allocated_frame.set_order(request_order);
                    allocated_frame.set_state(State::Alloc);
                }
            }

            return node_opt;
        }

        None
    }

    pub fn free_frame(&mut self, unique_node: Unique<Node<Frame>>) {
        let node = unsafe { &mut *unique_node.as_ptr() };
        let frame = node.as_mut();

        for order in frame.order()..MAX_ORDER {
            // Try to merge the buddy frames.
            let mut unique_buddy_node = self.get_buddy_node(unique_node, order);
            let buddy_node = unsafe { unique_buddy_node.as_mut() };
            {
                let buddy_frame = buddy_node.as_ref();
                if buddy_frame.state() == State::Free {
                    break;
                }
                self.count_free_frames[buddy_frame.order()] -= 1;
            }

            buddy_node.detach();
        }

        self.push_node_frame(frame.order(), unique_node);
    }
}


#[cfg(test)]
mod tests {
    use super::*;

    use std::heap::{Alloc, Layout, System};
    use std::mem;
    use std::slice;

    const FRAME_SIZE: usize = 4096;

    fn allocate_node_objs<'a, T>(count: usize) -> &'a mut [T]
    where
        T: Default,
    {
        let type_size = mem::size_of::<T>();
        let align = mem::align_of::<T>();
        let layout = Layout::from_size_align(count * type_size, align).unwrap();
        let ptr = unsafe { System.alloc(layout) }.unwrap();
        unsafe { slice::from_raw_parts_mut(ptr as *mut T, count) }
    }

    #[test]
    fn new_buddy_manager() {
        let mut expected_counts = [0usize; MAX_ORDER];

        let count = 1024;
        let nodes = allocate_node_objs::<Node<Frame>>(count);

        let bman = BuddyManager::new(&mut nodes[0] as *mut _, count, 0, FRAME_SIZE);
        expected_counts[10] += 1;
        assert_eq!(bman.count_free_frames, expected_counts);
    }

    #[test]
    fn test_get_frame_index() {
        let count = 1024;
        let nodes = allocate_node_objs::<Node<Frame>>(count);
        let bman = BuddyManager::new(&mut nodes[0] as *mut _, count, 0, FRAME_SIZE);

        assert_eq!(
            bman.get_frame_index(unsafe { Unique::new_unchecked(&mut nodes[0] as *mut _) }),
            0
        );
        assert_eq!(
            bman.get_frame_index(unsafe { Unique::new_unchecked(&mut nodes[10] as *mut _) }),
            10
        );
        assert_eq!(
            bman.get_frame_index(unsafe { Unique::new_unchecked(&mut nodes[1023] as *mut _) }),
            1023
        );
    }

    #[test]
    fn test_get_buddy_node() {
        let count = 1024;
        let nodes = allocate_node_objs::<Node<Frame>>(count);
        let _bman = BuddyManager::new(&mut nodes[0] as *mut _, count, 0, FRAME_SIZE);
    }

    #[test]
    fn test_allocate_frame_by_order_and_free() {
        let count = 1024;
        let nodes = allocate_node_objs::<Node<Frame>>(count);
        let mut bman = BuddyManager::new(&mut nodes[0] as *mut _, count, 0, FRAME_SIZE);

        assert_eq!(bman.count_free_frames(), 1024);

        let node = bman.allocate_frame_by_order(1).unwrap();
        assert_eq!(unsafe { node.as_ref() }.as_ref().order(), 1);
        assert_eq!(unsafe { node.as_ref() }.as_ref().state(), State::Alloc);

        assert_eq!(bman.count_free_frames(), 1022);

        let node = bman.allocate_frame_by_order(0).unwrap();
        assert_eq!(unsafe { node.as_ref() }.as_ref().order(), 0);
        assert_eq!(unsafe { node.as_ref() }.as_ref().state(), State::Alloc);
        assert_eq!(bman.count_free_frames(), 1021);
        assert_eq!(bman.count_used_frames(), 3);
        assert_eq!(bman.free_memory_size(), 1021 * FRAME_SIZE);
        assert_eq!(bman.used_memory_size(), 3 * FRAME_SIZE);

        bman.free_frame(node);
        assert_eq!(bman.count_free_frames(), 1022);

        let count = 512;
        let nodes = allocate_node_objs::<Node<Frame>>(count);
        let mut bman = BuddyManager::new(&mut nodes[0] as *mut _, count, 0, FRAME_SIZE);
        let node1 = bman.allocate_frame_by_order(1).unwrap();
        assert_eq!(bman.count_free_frames(), 510);
        assert_eq!(bman.count_used_frames(), 2);
        assert_eq!(bman.free_memory_size(), 510 * FRAME_SIZE);
        assert_eq!(bman.used_memory_size(), 2 * FRAME_SIZE);

        let _node2 = bman.allocate_frame_by_order(3).unwrap();
        assert_eq!(bman.count_free_frames(), 502);
        assert_eq!(bman.count_used_frames(), 10);

        bman.free_frame(node1);
        assert_eq!(bman.count_free_frames(), 504);
        assert_eq!(bman.count_used_frames(), 8);
    }

    #[test]
    fn test_get_addr() {
        let count = 128;
        let nodes = allocate_node_objs::<Node<Frame>>(count);
        let base_addr = 1024;
        let mut bman = BuddyManager::new(&mut nodes[0] as *mut _, count, base_addr, FRAME_SIZE);

        let node1 = bman.allocate_frame_by_order(0).unwrap();
        let node2 = bman.allocate_frame_by_order(0).unwrap();
        assert_eq!(bman.get_addr(node1), base_addr);
        assert_eq!(bman.get_addr(node2), base_addr + FRAME_SIZE);
    }
}
