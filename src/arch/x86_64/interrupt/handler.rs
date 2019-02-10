use bitfield::bitfield;
use static_assertions::assert_eq_size;

bitfield! {
    #[repr(C)]
    pub struct ErrorCode(u32);
    impl Debug;
    external_event, _: 0;
    descriptor_location, _: 1;
    ti, _: 2;
    segment_selector_index, _: 13, 3;
    reserved, _: 31, 14;
}

pub type Handler = extern "x86-interrupt" fn(error: ErrorCode);
assert_eq_size!(usize, Handler);

pub extern "x86-interrupt" fn sample_handler(_: ErrorCode) {
    println!("Got interrupt");
}
