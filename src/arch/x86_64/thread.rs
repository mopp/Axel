use super::interrupt::InterruptFrame;

pub struct Thread {}

impl Thread {
    pub fn new() -> Thread {
        Thread {}
    }
}

pub fn switch_context(frame: &mut InterruptFrame) {
    crate::process::select_threads(|| {
        frame.cs = 8 * 3 + 3;
        frame.ss = 8 * 4 + 3;
        unsafe {
            asm!("mov ds, $0
                  mov es, $0
                  mov fs, $0
                  mov gs, $0
                 "
                 :
                 : "r"(frame.ss as u16)
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
