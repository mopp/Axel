use super::descriptor::Descriptor;
use super::handler::{default_handler, default_handler_with_error_code, Handler, HandlerWithErrorCode};
use static_assertions::assert_eq_size;

#[repr(C, packed)]
struct InterruptDescriptorTableRegister {
    limit: u16,
    base: usize,
}
assert_eq_size!([u8; 10], InterruptDescriptorTableRegister);

impl InterruptDescriptorTableRegister {
    #[inline(always)]
    fn load(&self) {
        unsafe {
            asm!(".intel_syntax noprefix
                  lidt [$0]"
                  :
                  : "r"(self)
                  :
                  : "intel");
        }
    }
}

#[repr(C)]
pub struct InterruptDescriptorTable {
    divide_error_exception: Descriptor<Handler>,
    debug: Descriptor<Handler>,
    nonmaskable: Descriptor<Handler>,
    breakpoint: Descriptor<Handler>,
    overflow: Descriptor<Handler>,
    bound_range_exceeded: Descriptor<Handler>,
    invalid_opcode: Descriptor<Handler>,
    device_not_available: Descriptor<Handler>,
    double_fault: Descriptor<HandlerWithErrorCode>,
    coprocessor_segment_overrun: Descriptor<Handler>,
    invalid_tss: Descriptor<HandlerWithErrorCode>,
    segment_not_present: Descriptor<HandlerWithErrorCode>,
    stack_segment_not_present: Descriptor<HandlerWithErrorCode>,
    general_protection: Descriptor<HandlerWithErrorCode>,
    page_fault: Descriptor<HandlerWithErrorCode>,
    reserved: Descriptor<Handler>,
    x87_pfu_floting_point_error: Descriptor<Handler>,
    alignment_check_exception: Descriptor<HandlerWithErrorCode>,
    machine_check_exception: Descriptor<Handler>,
    simd_floating_point_exception: Descriptor<Handler>,
    virtualization_exception: Descriptor<Handler>,
    reserveds: [Descriptor<Handler>; 11],
    user_defined_interrupts: [Descriptor<Handler>; 224],
}
assert_eq_size!([u8; 16 * 256], InterruptDescriptorTable);

impl InterruptDescriptorTable {
    #[inline(always)]
    pub fn init(&mut self) {
        self.divide_error_exception = Descriptor::with_handler(default_handler);
        self.debug = Descriptor::with_handler(default_handler);
        self.nonmaskable = Descriptor::with_handler(default_handler);
        self.breakpoint = Descriptor::with_handler(default_handler);
        self.overflow = Descriptor::with_handler(default_handler);
        self.bound_range_exceeded = Descriptor::with_handler(default_handler);
        self.invalid_opcode = Descriptor::with_handler(default_handler);
        self.device_not_available = Descriptor::with_handler(default_handler);
        self.double_fault = Descriptor::with_handler_with_error_code(default_handler_with_error_code);
        self.coprocessor_segment_overrun = Descriptor::with_handler(default_handler);
        self.invalid_tss = Descriptor::with_handler_with_error_code(default_handler_with_error_code);
        self.segment_not_present = Descriptor::with_handler_with_error_code(default_handler_with_error_code);
        self.stack_segment_not_present = Descriptor::with_handler_with_error_code(default_handler_with_error_code);
        self.general_protection = Descriptor::with_handler_with_error_code(default_handler_with_error_code);
        self.page_fault = Descriptor::with_handler_with_error_code(default_handler_with_error_code);
        self.reserved = Descriptor::with_handler(default_handler);
        self.x87_pfu_floting_point_error = Descriptor::with_handler(default_handler);
        self.alignment_check_exception = Descriptor::with_handler_with_error_code(default_handler_with_error_code);
        self.machine_check_exception = Descriptor::with_handler(default_handler);
        self.simd_floating_point_exception = Descriptor::with_handler(default_handler);
        self.virtualization_exception = Descriptor::with_handler(default_handler);
        for i in self.reserveds.iter_mut() {
            *i = Descriptor::with_handler(default_handler)
        }
        for i in self.user_defined_interrupts.iter_mut() {
            *i = Descriptor::with_handler(default_handler)
        }
    }

    #[inline(always)]
    pub fn load(&self) {
        let idtr = InterruptDescriptorTableRegister {
            limit: core::mem::size_of::<Self>() as u16 - 1,
            base: (self as *const _) as usize,
        };

        idtr.load();
    }
}
