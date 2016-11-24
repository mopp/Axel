use graphic::CharacterDisplay;
use spin::Mutex;
use arch;

pub struct AxelContext {
    pub kernel_output_device: Mutex<Option<CharacterDisplay<'static>>>,
}


lazy_static! {
    pub static ref GLOBAL_CONTEXT: AxelContext = {
            AxelContext {
                kernel_output_device: Mutex::new(arch::obtain_kernel_console()),
            }
    };
}
