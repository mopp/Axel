use alist::AList;
use alist::Node;
use arch::x86_32::memory::frame;
use arch::x86_32::memory::frame::Frame;
use arch::x86_32::memory::frame::FrameState;
use core::mem;
use core::cmp;


// Order in buddy system : 0  1  2  3  4  5  6   7   8   9   10   11   12   13    14
// The number of frame   : 1  2  4  8 16 32 64 128 256 512 1024 2048 4096 8192 16384
pub const MAX_ORDER: usize = 14 + 1;

pub struct BuddyManager<'a> {
    pub frames: &'a mut [Node<Frame>],
    pub base_addr: usize,
    pub num_total_frames: usize,
    pub num_each_free_frames: &'a mut [usize],
    pub frame_lists: &'a mut [AList<Frame>],
}


#[allow(dead_code)]
impl<'a> BuddyManager<'a> {
    pub fn init(&mut self)
    {
        // Init all frames.
        for node in self.frames.iter_mut() {
            node.init(Default::default());
        }

        // Make groups by order.
        let mut num_alone_frames = self.num_total_frames;
        let mut idx = 0;
        for order in (0..MAX_ORDER).rev() {
            let num_frames_in_group = 1 << order;
            let mut cnt = 0;
            while (num_alone_frames != 0) && (num_frames_in_group <= num_alone_frames) {
                let node = &mut self.frames[idx];
                {
                    // Set frame order.
                    let frame = node.get_mut();
                    frame.order = order;
                }
                self.frame_lists[order].push_back(node);

                cnt += 1;
                idx += num_frames_in_group;
                num_alone_frames -= num_frames_in_group;
            }
            self.num_each_free_frames[order] = cnt;
        }
    }


    fn num_free_frames(&self) -> usize
    {
        let mut cnt = 0;
        for i in 0..MAX_ORDER {
            cnt += self.num_each_free_frames[i] * (1 << i)
        }

        cnt
    }


    fn total_memory_size(&self) -> usize
    {
        frame::SIZE * self.num_total_frames
    }


    fn free_memory_size(&self) -> usize
    {
        frame::SIZE * self.num_free_frames()
    }


    fn alloc_memory_size(&self) -> usize
    {
        self.total_memory_size() - self.free_memory_size()
    }


    fn frame_idx(&self, frame: &Node<Frame>) -> usize
    {
        let base_addr = addr_of_var!(self.frames[0]);
        let addr      = addr_of_ref!(frame);
        let size      = mem::size_of::<Node<Frame>>();
        (addr - base_addr) / size
    }


    pub fn frame_addr(&self, frame: &Node<Frame>) -> usize
    {
        self.base_addr + self.frame_idx(frame) * frame::SIZE
    }


    fn buddy_frame_idx(&self, frame: &Node<Frame>, order: usize) -> usize
    {
        (self.frame_idx(frame) ^ (1 << order))
    }


    pub fn alloc<'b>(&mut self, request_order: usize) -> Option<&'b mut Node<Frame>>
    {
        for order in request_order..MAX_ORDER  {
            if self.frame_lists[order].is_empty() {
                continue;
            }

            // Found an enough frame.
            // And detach the frame.
            let node = self.frame_lists[order].pop_front().unwrap();
            self.num_each_free_frames[order] -= 1;

            {
                // Change the frame configs.
                let detach_frame    = node.get_mut();
                detach_frame.status = FrameState::Alloc;
                detach_frame.order  = request_order;
            }

            // If the frame that has larger order than requested order, remain frames of the frame should be inserted into other frame lists.
            let mut order = order;
            while request_order < order {
                order -= 1;
                // Find buddy frame (a half part of the frame)
                let idx = self.buddy_frame_idx(node, order);
                {
                    let frame    = self.frames[idx].get_mut();
                    frame.status = FrameState::Free;
                    frame.order  = order;
                }
                self.frame_lists[order].push_front(&mut self.frames[idx]);
                self.num_each_free_frames[order] += 1;
            }

            return Some(node);
        }

        None
    }


    pub fn disjoint_frame(&mut self, idx: usize)
    {
        let order = self.frames[idx].get().order;
        self.num_each_free_frames[order] -= 1;
        self.frames[idx].disjoint(&mut self.frame_lists[order]);
    }


    pub fn free(&mut self, node: &mut Node<Frame>)
    {
        // If buddy frame is free, the two frames should be merged.
        // And this procedure is repeated until buddy frame is not free.
        let mut idx = self.frame_idx(node);
        loop {
            let (buddy_idx, order) = {
                let free_frame  = &self.frames[idx];
                let order = free_frame.get().order;
                (self.buddy_frame_idx(free_frame, order), order)
            };

            if self.num_total_frames < buddy_idx  {
                break;
            }

            {
                let buddy_node = &mut self.frames[buddy_idx];
                let buddy_frame = buddy_node.get();
                if buddy_frame.is_alloc() || (order != buddy_frame.order) {
                    // The frame cannot be merged.
                    break;
                }
            };

            // Disjoint buddy frame.
            self.disjoint_frame(buddy_idx);

            // Merge buddy frame and the frame.
            idx = cmp::min(idx, buddy_idx);
            let merged_frame = &mut self.frames[idx];
            merged_frame.get_mut().order = order + 1;
        };

        let order = self.frames[idx].get_mut().order;
        self.frame_lists[order].push_front(&mut self.frames[idx]);
        self.num_each_free_frames[order] += 1;
    }
}
