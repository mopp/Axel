use bitfield::bitfield;
use core::fmt;

bitfield! {
    #[repr(C)]
    pub struct ErrorCode(u64);
    impl Debug;
    external_event, _: 0;
    descriptor_location, _: 1;
    ti, _: 2;
    segment_selector_index, _: 13, 3;
    reserved, _: 31, 14;
}

#[repr(C)]
pub struct InterruptFrame {
    ip: usize,
    pub cs: usize,
    flags: usize,
    sp: usize,
    pub ss: usize,
}

impl fmt::Debug for InterruptFrame {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "InterruptFrame {{ cs:ip = 0x{:x}:0x{:x}, ss:sp = 0x{:x}:0x{:x}, flags = 0x{:x}}}", self.cs, self.ip, self.ss, self.sp, self.flags)
    }
}

pub type Handler = extern "x86-interrupt" fn(&mut InterruptFrame);
pub type HandlerWithErrorCode = extern "x86-interrupt" fn(&mut InterruptFrame, ErrorCode);

pub extern "x86-interrupt" fn default_handler(interrupt_frame: &mut InterruptFrame) {
    println!("Got interrupt");
    println!("  {:?}", interrupt_frame);
}

pub extern "x86-interrupt" fn default_handler_with_error_code(interrupt_frame: &mut InterruptFrame, error: ErrorCode) {
    println!("Got interrupt");
    println!("  {:?}", interrupt_frame);
    println!("  {:x}", error.0);
}
