#![feature(asm)]
#![feature(collections)]
#![feature(alloc)]
#![feature(conservative_impl_trait)]
#![feature(global_allocator)]
#![feature(lang_items)]
#![feature(shared)]
#![feature(start)]
#![no_std]

#[cfg(test)]
#[macro_use]
extern crate std;

extern crate alloc;

#[cfg(not(test))]
extern crate kernel_memory_allocator;
use kernel_memory_allocator::KernelMemoryAllocator;

#[global_allocator]
static GLOBAL: KernelMemoryAllocator = KernelMemoryAllocator;

// #[macro_use]
extern crate bitflags;

#[macro_use]
extern crate lazy_static;

// extern crate alloc;
extern crate multiboot2;
extern crate rlibc;
extern crate spin;

#[cfg(not(test))]
#[macro_use]
mod log;

#[macro_use]
mod memory;

mod alist;
mod arch;
mod context;
mod graphic;


#[cfg(not(test))]
#[start]
#[no_mangle]
pub extern fn main(argc: usize, argv: *const usize)
{
    memory::clean_bss_section();

    let argv: &[usize] = unsafe { core::slice::from_raw_parts(argv, argc) };

    // Initialize stuffs depending on the architecture.
    arch::init_arch(argv);

    memory::init();

    println!("Start Axel");

    let ref mut memory_region_manager = *context::GLOBAL_CONTEXT.memory_region_manager.lock();
    for region in memory_region_manager.regions_iter_with(memory::region::State::Free) {
        println!("Base addr : 0x{:08X}", region.base_addr());
        println!("Size      : {}KB", region.size() / 1024);
    }

    {
        use alloc::boxed::Box;
        let heap_test = Box::new(42);

        use alloc::LinkedList;
        let list: LinkedList<u32> = LinkedList::new();
    }
}


#[cfg(not(test))]
#[lang = "eh_personality"]
pub extern fn eh_personality()
{
}


#[cfg(not(test))]
#[lang = "panic_fmt"]
#[no_mangle]
pub extern fn panic_fmt(msg: core::fmt::Arguments, file: &'static str, line: u32) -> !
{
    println!("Panic - {}", msg);
    println!("Line {} in {}", line, file);
    loop {}
}


#[no_mangle]
pub extern fn abort()
{
    println!("abort");
    loop {}
}
