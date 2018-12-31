use self::super::arch;
use self::super::arch::Initialize;
use self::super::graphic::CharacterDisplay;
use spin::Mutex;

pub struct Context {
    pub default_output: Mutex<Option<CharacterDisplay<'static>>>,
}

lazy_static! {
    pub static ref GLOBAL_CONTEXT: Context = {
        Context {
            default_output: Mutex::new(arch::Initializer::obtain_kernel_console()),
        }
    };
}
