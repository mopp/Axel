use super::Object;
use crate::memory::{Frame, FrameAllocator};
use core::mem;
use core::ptr;
use core::ptr::Unique;
use intrusive_collections::{Adapter, IntrusivePointer, LinkedList, LinkedListLink, UnsafeRef};

const MAX_ORDER: usize = 15;

pub struct BuddyAllocator<A: Clone + Adapter<Link = LinkedListLink, Pointer = UnsafeRef<T>, Value = T>, T: Object> {
    obj_ptr: Unique<T>,
    obj_count: usize,
    free_lists: [LinkedList<A>; MAX_ORDER],
    free_counts: [usize; MAX_ORDER],
}

impl<A: Clone + Adapter<Link = LinkedListLink, Pointer = UnsafeRef<T>, Value = T>, T: Object> BuddyAllocator<A, T> {
    pub fn new(obj_ptr: Unique<T>, count: usize, adapter: A) -> BuddyAllocator<A, T> {
        let mut free_lists = unsafe {
            let mut lists: [LinkedList<A>; MAX_ORDER] = mem::uninitialized();

            for l in lists.iter_mut() {
                ptr::write(l, LinkedList::new(adapter.clone()))
            }

            lists
        };

        // Insert each object into list which has suitable order.
        let mut index = 0;
        let mut free_counts = [0; MAX_ORDER];
        let objs = unsafe { core::slice::from_raw_parts_mut(obj_ptr.as_ptr(), count) };

        for i in 0..count {
            objs[i].reset_link();
        }

        for order in (0..MAX_ORDER).rev() {
            let count_in_order = 1 << order;
            while count_in_order <= (count - index) {
                // Initialize object.
                let obj = &mut objs[index];
                obj.set_order(order);
                obj.mark_free();

                free_lists[order].push_back(unsafe { IntrusivePointer::from_raw(obj as _) });
                free_counts[order] += 1;

                index += count_in_order;
            }
        }

        BuddyAllocator {
            obj_ptr: obj_ptr,
            obj_count: count,
            free_lists: free_lists,
            free_counts: free_counts,
        }
    }

    fn is_managed_obj(&self, ptr: *const T) -> bool {
        let head_addr = self.obj_ptr.as_ptr() as usize;
        let tail_addr = unsafe { self.obj_ptr.as_ptr().offset((self.obj_count - 1) as isize) as usize };

        let addr = ptr as usize;
        (head_addr <= addr) && (addr <= tail_addr)
    }

    fn buddy(&self, obj: UnsafeRef<T>, order: usize) -> Option<UnsafeRef<T>> {
        let raw = obj.into_raw();
        debug_assert!(self.is_managed_obj(raw), "The given object is out of range");

        if MAX_ORDER <= order {
            return None;
        }

        let index = raw.wrapping_offset_from(self.obj_ptr.as_ptr());
        if (index < 0) || ((self.obj_count as isize) < index) {
            None
        } else {
            let buddy_index = index ^ (1 << (order as isize));
            let addr = unsafe { self.obj_ptr.as_ptr().offset(buddy_index as isize) };

            if self.is_managed_obj(addr) {
                Some(unsafe { UnsafeRef::<T>::from_raw(addr) })
            } else {
                None
            }
        }
    }

    pub fn allocate(&mut self, request_order: usize) -> Option<UnsafeRef<T>> {
        if MAX_ORDER <= request_order {
            return None;
        }

        // Find last set instruction makes it more accelerate ?
        // 0001 1000
        // fls(map >> request_order) ?
        for order in request_order..MAX_ORDER {
            let obj = match self.free_lists[order].front_mut().remove() {
                Some(obj) => {
                    self.free_counts[order] -= 1;

                    obj.set_order(request_order);
                    obj.mark_used();
                    obj
                }
                None => continue,
            };

            // Push the extra frames.
            for i in request_order..order {
                if let Some(buddy_obj) = self.buddy(obj.clone(), i) {
                    buddy_obj.set_order(i);
                    buddy_obj.mark_free();

                    self.free_lists[i].push_back(buddy_obj);
                    self.free_counts[i] += 1;
                } else {
                    break;
                }
            }

            return Some(obj);
        }

        None
    }

    pub fn free(&mut self, mut obj: UnsafeRef<T>) {
        debug_assert!(self.is_managed_obj(obj.clone().into_raw()), "The given object is out of range");

        obj.mark_free();

        for i in obj.order()..MAX_ORDER {
            if let Some(buddy_obj) = self.buddy(obj.clone(), i) {
                // println!("obj: {:p} vs buddy: {:p}", obj.clone().into_raw(), buddy_obj.clone().into_raw());
                if buddy_obj.is_used() {
                    break;
                }
                // Merge the two object into one large object.

                // Take out the free buddy object.
                let buddy_obj = unsafe { self.free_lists[i].cursor_mut_from_ptr(buddy_obj.into_raw()).remove().unwrap() };
                self.free_counts[i] -= 1;

                // Select object which has smaller address.
                if buddy_obj.clone().into_raw() < obj.clone().into_raw() {
                    obj = buddy_obj;
                    obj.mark_free();
                }

                obj.set_order(i + 1)
            } else {
                break;
            }
        }

        let order = obj.order();
        self.free_lists[order].push_back(obj);
        self.free_counts[order] += 1;
    }

    pub fn count_free_objs(&self) -> usize {
        self.free_counts.iter().enumerate().fold(0, |acc, (order, count)| acc + count * (1 << order))
    }
}

impl<A: Clone + Adapter<Link = LinkedListLink, Pointer = UnsafeRef<Frame>, Value = Frame>> FrameAllocator for BuddyAllocator<A, Frame> {
    fn alloc_one(&mut self) -> Option<Frame> {
        self.allocate(1)
            .map(|frame| (*frame).clone())
    }

    fn free(&mut self, f: Frame) {
        unsafe {
            let addr = self.obj_ptr.as_ptr().offset(f.number() as isize);
            self.free(UnsafeRef::from_raw(addr));
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use memory::frame::{Frame, FrameAdapter};
    use std::alloc::{Alloc, Layout, System};
    use std::mem;

    fn allocate_nodes<T>(count: usize) -> *mut T {
        let type_size = mem::size_of::<T>();
        let align = mem::align_of::<T>();
        let layout = Layout::from_size_align(count * type_size, align).unwrap();
        let ptr = unsafe { System.alloc(layout) }.unwrap();

        ptr.as_ptr() as *mut _
    }

    fn make_frame_allocator(count: usize) -> (*mut Frame, BuddyAllocator<FrameAdapter, Frame>) {
        let nodes = allocate_nodes(count);
        let allocator = BuddyAllocator::new(unsafe { Unique::new_unchecked(nodes) }, count, FrameAdapter::new());
        (nodes, allocator)
    }

    #[test]
    fn test_allocate_simple_split() {
        const SIZE: usize = 32;
        let (_, mut allocator) = make_frame_allocator(SIZE);

        assert_eq!(SIZE, allocator.count_free_objs());

        let obj = allocator.allocate(4);
        assert!(obj.is_some());
        assert_eq!(SIZE - (1 << 4), allocator.count_free_objs());
        for i in 0..MAX_ORDER {
            if i == 4 {
                assert_eq!(1, allocator.free_counts[i]);
            } else {
                assert_eq!(0, allocator.free_counts[i]);
            }
        }

        allocator.free(obj.unwrap());
        assert_eq!(SIZE, allocator.count_free_objs());
        for i in 0..MAX_ORDER {
            if i == 5 {
                assert_eq!(1, allocator.free_counts[i]);
            } else {
                assert_eq!(0, allocator.free_counts[i]);
            }
        }
    }

    #[test]
    fn test_allocate_and_free() {
        const SIZE: usize = 32;
        let (_, mut allocator) = make_frame_allocator(SIZE);

        assert_eq!(SIZE, allocator.count_free_objs());

        if let Some(obj) = allocator.allocate(2) {
            assert_eq!(2, obj.order());
            assert_eq!(SIZE - 4, allocator.count_free_objs());

            allocator.free(obj);
            assert_eq!(SIZE, allocator.count_free_objs());
        } else {
            assert!(false)
        }
    }

    #[test]
    fn test_try_to_allocate_all() {
        const SIZE: usize = 32;
        let (_, mut allocator) = make_frame_allocator(SIZE);

        let mut list = LinkedList::new(FrameAdapter::new());

        // Allocate the all objects.
        while let Some(obj) = allocator.allocate(0) {
            assert_eq!(0, obj.order());
            list.push_back(obj);
        }

        assert_eq!(true, allocator.allocate(0).is_none());
        assert_eq!(0, allocator.count_free_objs());
        assert_eq!(SIZE, list.iter().count());

        // Deallocate the all objects.
        for obj in list.into_iter() {
            allocator.free(obj);
        }

        assert_eq!(SIZE, allocator.count_free_objs());
    }

    #[test]
    fn test_is_managed_obj() {
        const SIZE: usize = 32;
        let (nodes, mut allocator) = make_frame_allocator(SIZE);

        unsafe {
            // The first node.
            assert_eq!(true, allocator.is_managed_obj(nodes));
            // The last node.
            assert_eq!(true, allocator.is_managed_obj(nodes.offset((SIZE - 1) as isize)));
            // Invalid nodes.
            assert_eq!(false, allocator.is_managed_obj(nodes.offset(-1)));
            assert_eq!(false, allocator.is_managed_obj(nodes.offset(SIZE as isize)));
        }
    }

    #[test]
    fn test_buddy() {
        const SIZE: usize = 1 << MAX_ORDER;
        let (nodes, mut allocator) = make_frame_allocator(SIZE);

        let compare_buddy = |expected_buddy_ptr: *mut Frame, (node, order): (UnsafeRef<Frame>, usize)| -> bool {
            if let Some(buddy) = allocator.buddy(node, order) {
                ptr::eq(expected_buddy_ptr, buddy.into_raw())
            } else {
                false
            }
        };

        unsafe {
            let n0: UnsafeRef<Frame> = UnsafeRef::from_raw(nodes as *const _);
            for i in 0..MAX_ORDER {
                assert_eq!(true, compare_buddy(nodes.offset(1 << i), (n0.clone(), i)));
            }
            assert_eq!(false, compare_buddy(nodes.offset(0), (n0, MAX_ORDER)));

            let n1: UnsafeRef<Frame> = UnsafeRef::from_raw(nodes.offset(1) as *const _);
            assert_eq!(true, compare_buddy(nodes.offset(0), (n1.clone(), 0)));
            for i in 1..MAX_ORDER {
                assert_eq!(true, compare_buddy(nodes.offset((1 << i) + 1), (n1.clone(), i)));
            }
            assert_eq!(false, compare_buddy(nodes.offset(0), (n1, MAX_ORDER)));
        }
    }
}
