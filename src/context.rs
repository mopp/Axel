use arch;
use graphic::CharacterDisplay;
use spin::Mutex;

pub struct Context {
    pub default_output: Mutex<Option<CharacterDisplay<'static>>>,
}

lazy_static! {
    pub static ref GLOBAL_CONTEXT: Context = {
        Context {
            default_output: Mutex::new(arch::obtain_kernel_console()),
        }
    };
}
