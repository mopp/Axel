use graphic::CharacterDisplay;
use core::fmt::Write;

pub struct AxelContext {
    pub kernel_output_device: Option<CharacterDisplay<'static>>,
}


pub static mut AXEL_CONTEXT: AxelContext = AxelContext {
    kernel_output_device: None,
};
