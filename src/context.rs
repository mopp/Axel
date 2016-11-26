use arch;
use graphic::CharacterDisplay;
use memory;
use spin::Mutex;


pub struct Context {
    pub kernel_output_device: Mutex<Option<CharacterDisplay<'static>>>,
    pub memory_region_manager: Mutex<memory::region::RegionManager>,
}


lazy_static! {
    pub static ref GLOBAL_CONTEXT: Context = {
        Context {
            kernel_output_device: Mutex::new(arch::obtain_kernel_console()),
            memory_region_manager: Mutex::new(memory::region::RegionManager::new()),
        }
    };
}
