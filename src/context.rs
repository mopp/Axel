use arch;
use graphic::CharacterDisplay;
use memory;
use spin::Mutex;

pub struct Context {
    pub default_output: Mutex<Option<CharacterDisplay<'static>>>,
    pub memory_region_manager: Mutex<memory::region::RegionManager>,
}

lazy_static! {
    pub static ref GLOBAL_CONTEXT: Context = {
        Context {
            default_output: Mutex::new(arch::obtain_kernel_console()),
            memory_region_manager: Mutex::new(memory::region::RegionManager::new()),
        }
    };
}
