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

pub fn switch_process() {
    // let mut current_ip = 0;
    // let mut current_sp = 0;
    // let mut next_ip = 0;
    // let mut next_sp = 0;
    // unsafe {
    //     asm!("
    //         pushf
    //
    //         mov [$0], $2
    //         mov [$1], rsp
    //         "
    //               : "=r"(&mut current_ip) "=r"(&mut current_sp)
    //               : "r"(landing_point as usize)
    //               : "memory"
    //               : "intel", "volatile");
    // }
    //
    // println!("{}", current_ip);
    // println!("{}", current_sp);
}

// #[no_mangle]
// pub extern "C" fn landing_point() {
//     unsafe { asm!("popf") }
// }
