use bitfield::bitfield;

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
    cs: usize,
    flags: usize,
    sp: usize,
    ss: usize,
}

pub type Handler = extern "x86-interrupt" fn(&InterruptFrame);
pub type HandlerWithErrorCode = extern "x86-interrupt" fn(&InterruptFrame, ErrorCode);

pub extern "x86-interrupt" fn default_handler(interrupt_frame: &InterruptFrame) {
    println!("Got interrupt");
}

pub extern "x86-interrupt" fn default_handler_with_error_code(interrupt_frame: &InterruptFrame, error: ErrorCode) {
    println!("Got interrupt");
}
