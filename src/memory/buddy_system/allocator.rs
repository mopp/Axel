use super::Object;
use core::mem;
use core::ptr;
use core::ptr::{NonNull, Unique};
use intrusive_collections::{Adapter, IntrusivePointer, LinkedList, LinkedListLink};

const MAX_ORDER: usize = 15;

pub struct BuddyAllocator<A: Clone + Adapter<Link = LinkedListLink, Pointer = U, Value = T>, U: IntrusivePointer<T>, T: Object> {
    obj_ptr: Unique<T>,
    obj_count: usize,
    free_lists: [LinkedList<A>; MAX_ORDER],
    free_counts: [usize; MAX_ORDER],
}

impl<A: Clone + Adapter<Link = LinkedListLink, Pointer = U, Value = T>, U: IntrusivePointer<T>, T: Object> BuddyAllocator<A, U, T> {
    pub fn new(obj_ptr: Unique<T>, count: usize, adapter: A) -> BuddyAllocator<A, U, T> {
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
        for order in (0..MAX_ORDER).rev() {
            let count_in_order = 1 << order;
            while count_in_order <= (count - index) {
                // Initialize object.
                let obj = &mut objs[index];
                obj.set_order(order);
                obj.mark_free();
                obj.reset_link();

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

    // fn is_managed_obj(&self, ptr: *const T) -> bool {
    //     let head_addr = self.obj_ptr.as_ptr() as usize;
    //     let tail_addr = unsafe { self.obj_ptr.as_ptr().offset((self.obj_count - 1) as isize) as usize };
    //
    //     let addr = ptr as usize;
    //     (head_addr <= addr) && (addr <= tail_addr)
    // }
    //
    // fn buddy(&self, obj: Unique<T>, order: usize) -> Option<NonNull<T>> {
    //     debug_assert!(self.is_managed_obj(obj.as_ptr()), "The given object is out of range");
    //
    //     if MAX_ORDER <= order {
    //         return None;
    //     }
    //
    //     let index = obj.as_ptr().wrapping_offset_from(self.obj_ptr.as_ptr());
    //     if (index < 0) || ((self.obj_count as isize) < index) {
    //         None
    //     } else {
    //         let buddy_index = index ^ (1 << (order as isize));
    //         let buddy_obj = unsafe {
    //             let addr = self.obj_ptr.as_ptr().offset(buddy_index as isize);
    //             NonNull::new_unchecked(addr)
    //         };
    //
    //         if self.is_managed_obj(buddy_obj.as_ptr()) {
    //             Some(buddy_obj)
    //         } else {
    //             None
    //         }
    //     }
    // }

    pub fn allocate(&mut self) -> Option<U> {
        let mut cur = self.free_lists[0].cursor_mut();
        cur.move_next();
        cur.remove()
    }

    // pub fn allocate(&mut self, request_order: usize) -> Option<Unique<T>> {
    //     if MAX_ORDER <= request_order {
    //         return None;
    //     }
    //
    //     // Find last set instruction makes it more accelerate ?
    //     // 0001 1000
    //     // fls(map >> request_order) ?
    //     for order in request_order..MAX_ORDER {
    //         match self.free_lists[order].pop_front() {
    //             None => continue,
    //             Some(mut obj) => {
    //                 unsafe {
    //                     obj.set_order(request_order);
    //                     obj.mark_used();
    //                 };
    //
    //                 self.free_counts[order] -= 1;
    //
    //                 // Push the extra frames.
    //                 for i in request_order..order {
    //                     match self.buddy(obj, i) {
    //                         Some(mut buddy_obj) => {
    //                             unsafe {
    //                                 let buddy_obj = buddy_obj.as_mut();
    //                                 buddy_obj.set_order(i);
    //                                 buddy_obj.mark_free();
    //                             }
    //                             let buddy_obj = Unique::from(buddy_obj);
    //                             self.free_lists[i].push_back(buddy_obj);
    //                             self.free_counts[i] += 1;
    //                         }
    //                         None => {
    //                             break;
    //                         }
    //                     }
    //                 }
    //
    //                 return Some(obj);
    //             }
    //         }
    //     }
    //
    //     None
    // }


    // pub fn free(&mut self, mut obj: Unique<T>) {
    //     debug_assert!(self.is_managed_obj(obj.as_ptr()), "The given object is out of range");
    //
    //     let order = unsafe {
    //         obj.as_mut().mark_free();
    //         obj.as_ref().order()
    //     };
    //
    //     for i in order..MAX_ORDER {
    //         if let Some(mut buddy_obj) = self.buddy(obj, i) {
    //             if unsafe { buddy_obj.as_ref().is_used() } {
    //                 break;
    //             }
    //             // Merge the two object into one large object.
    //
    //             // Take out the free buddy object.
    //             self.free_lists[i].detach(buddy_obj);
    //             self.free_counts[i] -= 1;
    //             let buddy_obj = Unique::from(buddy_obj);
    //
    //             // Select object which has smaller address.
    //             if buddy_obj.as_ptr() < obj.as_ptr() {
    //                 obj = buddy_obj;
    //             }
    //
    //             unsafe {
    //                 let obj = obj.as_mut();
    //                 obj.mark_free();
    //                 obj.set_order(i + 1)
    //             };
    //         } else {
    //             break;
    //         }
    //     }
    //
    //     let order = unsafe { obj.as_ref().order() };
    //     self.free_lists[order].push_tail(obj);
    //     self.free_counts[order] += 1;
    // }

    pub fn count_free_objs(&self) -> usize {
        self.free_counts.iter().enumerate().fold(0, |acc, (order, count)| acc + count * (1 << order))
    }
}

// #[cfg(test)]
// mod tests {
//     use super::*;
//     use list::Node;
//     use std::alloc::{Alloc, Layout, System};
//     use std::mem;
//
//     struct Frame {
//         next: Option<NonNull<Frame>>,
//         prev: Option<NonNull<Frame>>,
//         order: usize,
//         is_used: bool,
//     }
//
//     impl Node<Frame> for Frame {
//         fn set_next(&mut self, ptr: Option<NonNull<Self>>) {
//             self.next = ptr;
//         }
//
//         fn set_prev(&mut self, ptr: Option<NonNull<Self>>) {
//             self.prev = ptr;
//         }
//
//         fn next(&self) -> Option<NonNull<Self>> {
//             self.next
//         }
//
//         fn prev(&self) -> Option<NonNull<Self>> {
//             self.prev
//         }
//     }
//
//     impl Object for Frame {
//         fn mark_used(&mut self) {
//             self.is_used = true;
//         }
//
//         fn mark_free(&mut self) {
//             self.is_used = false;
//         }
//
//         fn is_used(&self) -> bool {
//             self.is_used
//         }
//
//         fn order(&self) -> usize {
//             self.order
//         }
//
//         fn set_order(&mut self, order: usize) {
//             self.order = order;
//         }
//     }
//
//     fn allocate_nodes<T>(count: usize) -> *mut T {
//         let type_size = mem::size_of::<T>();
//         let align = mem::align_of::<T>();
//         let layout = Layout::from_size_align(count * type_size, align).unwrap();
//         let ptr = unsafe { System.alloc(layout) }.unwrap();
//
//         ptr.as_ptr() as *mut _
//     }
//
//     #[test]
//     fn test_allocate_and_free() {
//         const SIZE: usize = 32;
//         let nodes = allocate_nodes(SIZE);
//         let nodes = unsafe { Unique::new_unchecked(nodes) };
//         let mut allocator: BuddyAllocator<Frame> = BuddyAllocator::new(nodes, SIZE);
//
//         assert_eq!(SIZE, allocator.count_free_objs());
//
//         if let Some(obj) = allocator.allocate(2) {
//             assert_eq!(2, unsafe { obj.as_ref().order() });
//
//             assert_eq!(SIZE - 4, allocator.count_free_objs());
//
//             allocator.free(obj);
//             assert_eq!(SIZE, allocator.count_free_objs());
//         }
//     }
//
//     #[test]
//     fn test_try_to_allocate_all() {
//         const SIZE: usize = 32;
//         let nodes = allocate_nodes(SIZE);
//         let nodes = unsafe { Unique::new_unchecked(nodes) };
//         let mut allocator: BuddyAllocator<Frame> = BuddyAllocator::new(nodes, SIZE);
//
//         let mut list = LinkedList::new();
//
//         // Allocate the all objects.
//         loop {
//             if let Some(obj) = allocator.allocate(0) {
//                 assert_eq!(0, unsafe { obj.as_ref().order() });
//                 list.push_tail(obj);
//             } else {
//                 break;
//             }
//         }
//
//         assert_eq!(true, allocator.allocate(0).is_none());
//         assert_eq!(0, allocator.count_free_objs());
//         assert_eq!(SIZE, list.count());
//
//         // Deallocate the all objects.
//         loop {
//             if let Some(obj) = list.pop_head() {
//                 allocator.free(obj);
//             } else {
//                 break;
//             }
//         }
//
//         assert_eq!(SIZE, allocator.count_free_objs());
//         assert_eq!(0, list.count());
//     }
//
//     #[test]
//     fn test_is_managed_obj() {
//         const SIZE: usize = 32;
//         let nodes = allocate_nodes(SIZE);
//         let nodes_unique = unsafe { Unique::new_unchecked(nodes) };
//         let allocator: BuddyAllocator<Frame> = BuddyAllocator::new(nodes_unique, SIZE);
//
//         unsafe {
//             // The first node.
//             assert_eq!(true, allocator.is_managed_obj(nodes));
//             // The last node.
//             assert_eq!(true, allocator.is_managed_obj(nodes.offset((SIZE - 1) as isize)));
//             // Invalid nodes.
//             assert_eq!(false, allocator.is_managed_obj(nodes.offset(-1)));
//             assert_eq!(false, allocator.is_managed_obj(nodes.offset(SIZE as isize)));
//         }
//     }
//
//     #[test]
//     fn test_buddy() {
//         const SIZE: usize = 1 << MAX_ORDER;
//         let nodes = allocate_nodes(SIZE);
//         let nodes_unique = unsafe { Unique::new_unchecked(nodes) };
//         let allocator: BuddyAllocator<Frame> = BuddyAllocator::new(nodes_unique, SIZE);
//
//         let compare_buddy = |expected_buddy_ptr: *mut Frame, (node, order): (Unique<Frame>, usize)| -> bool {
//             if let Some(buddy) = allocator.buddy(node, order) {
//                 ptr::eq(expected_buddy_ptr, buddy.as_ptr())
//             } else {
//                 false
//             }
//         };
//
//         unsafe {
//             let n0 = nodes_unique;
//             for i in 0..MAX_ORDER {
//                 assert_eq!(true, compare_buddy(nodes.offset(1 << i), (n0, i)));
//             }
//             assert_eq!(false, compare_buddy(nodes.offset(0), (n0, MAX_ORDER)));
//
//             let n1 = Unique::new_unchecked(nodes.offset(1));
//             assert_eq!(true, compare_buddy(nodes.offset(0), (n1, 0)));
//             for i in 1..MAX_ORDER {
//                 assert_eq!(true, compare_buddy(nodes.offset((1 << i) + 1), (n1, i)));
//             }
//             assert_eq!(false, compare_buddy(nodes.offset(0), (n1, MAX_ORDER)));
//         }
//     }
// }
