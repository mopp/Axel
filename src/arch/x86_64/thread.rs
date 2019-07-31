use super::interrupt::InterruptFrame;
use crate::process;

pub struct Thread {
    // interrupt_frame: InterruptFrame,
}

impl Thread {
    // pub fn new() -> Thread {
    //     Thread { interrupt_frame: InterruptFrame::new() }
    // }
}

// Steps
//   1. Change segment selector to user mode.
//   2. Keep the current execution context.
pub fn switch_context(current_interrupt_frame: &mut InterruptFrame) {
    process::switch_context(|_current_thread: &mut Thread, _next_thread: &mut Thread| {
        current_interrupt_frame.cs = 8 * 3 + 3;
        current_interrupt_frame.ss = 8 * 4 + 3;
        unsafe {
            asm!("mov ds, $0
                  mov es, $0
                  mov fs, $0
                  mov gs, $0
                 "
                 :
                 : "r"(current_interrupt_frame.ss as u16)
                 : "ax"
                 : "intel", "volatile"
            );
        }

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
    })
}

// #[no_mangle]
// pub extern "C" fn landing_point() {
//     unsafe { asm!("popf") }
// }
