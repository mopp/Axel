use graphic::CharacterDisplay;
use spin::Mutex;
use arch;

pub struct Context {
    pub kernel_output_device: Mutex<Option<CharacterDisplay<'static>>>,
}


lazy_static! {
    pub static ref GLOBAL_CONTEXT: Context = {
            Context {
                kernel_output_device: Mutex::new(arch::obtain_kernel_console()),
            }
    };
}
