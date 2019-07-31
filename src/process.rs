use crate::arch::Thread;
// use crate::memory::{ActivePageTable, FrameAllocator, InActivePageTable, ACTIVE_PAGE_TABLE};
// use alloc::boxed::Box;
use core::mem;
use core::ptr;
use lazy_static::lazy_static;
use spin::Mutex;

struct Process {
    // page_table: InActivePageTable,
    thread: Thread,
}
//
// impl Process {
//     fn new(active_page_table: &mut ActivePageTable, allocator: &mut FrameAllocator) -> Process {
//         // FIXME: remove 1st argument (ActivePageTable).
//         Process {
//             page_table: InActivePageTable::new(active_page_table, allocator).unwrap(),
//             thread: Thread::new(),
//         }
//     }
// }

const MAX_PROCESS_COUNT: usize = 16;

pub struct ProcessManager {
    // current_process: Box<Process>,
    processes: [Option<Process>; MAX_PROCESS_COUNT],
    // current_index: usize,
}

impl ProcessManager {
    fn new() -> ProcessManager {
        unsafe {
            // FIXME: `mem::uninitialized()` causes kernel stack overflow if the MAX_PROCESS_COUNT
            // is too large.
            let mut processes: [Option<Process>; MAX_PROCESS_COUNT] = mem::uninitialized();
            for p in processes.iter_mut() {
                ptr::write(p, None)
            }

            // processes[0] = create_init_user_process();

            ProcessManager { processes }
        }
    }
}

lazy_static! {
    static ref PROCESS_MANAGER: Mutex<ProcessManager> = Mutex::new(ProcessManager::new());
}

pub fn switch_context<F>(mut switch_thread: F)
where
    F: FnMut(&mut Thread, &mut Thread),
{
    // FIXME:
    let manager = &mut (*(*PROCESS_MANAGER).lock());
    let (m, n) = manager.processes.split_at_mut(1);
    if let Some(ref mut current) = m[0] {
        if let Some(ref mut next) = n[0] {
            switch_thread(&mut current.thread, &mut next.thread);
        }
    }
}

// fn create_init_user_process() -> Process {
//     Process::new()
// }
