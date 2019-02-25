use crate::memory::{ActivePageTable, FrameAllocator, InActivePageTable};
use core::mem;
use core::ptr;
use lazy_static::lazy_static;
use spin::Mutex;

struct Process {
    page_table: InActivePageTable,
}

impl Process {
    fn new(allocator: &mut FrameAllocator) -> Process {
        // FIXME: remove 1st argument (ActivePageTable).
        Process {
            page_table: InActivePageTable::new(unsafe { &mut ActivePageTable::new() }, allocator).unwrap(),
        }
    }
}

const MAX_PROCESS_COUNT: usize = 255;

pub struct ProcessManager {
    processes: [Option<Process>; MAX_PROCESS_COUNT],
    current_index: usize,
}

impl ProcessManager {
    fn new() -> ProcessManager {
        unsafe {
            let mut processes: [Option<Process>; MAX_PROCESS_COUNT] = mem::uninitialized();
            for p in processes.iter_mut() {
                ptr::write(p, None)
            }
            ProcessManager { processes, current_index: 0 }
        }
    }
}

lazy_static! {
    static ref PROCESS_MANAGER: Mutex<ProcessManager> = Mutex::new(ProcessManager::new());
}
