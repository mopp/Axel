//! This module contains all codes depending on the architecture to abstract these codes.

use core::slice;

#[cfg(target_arch = "arm")]
mod arm11;


#[cfg(target_arch = "arm")]
pub fn init_arch(_: usize, _: *const usize)
{
    arm11::init();
}



#[cfg(target_arch = "x86")]
mod x86_32;


#[cfg(target_arch = "x86")]
pub fn init_arch(argc: usize, argv: *const usize)
{
    x86_32::init(argc, argv);
}


// #[cfg(target_arch = "x86_64")]
// mod x86_64;


#[cfg(target_arch = "x86_64")]
pub fn init_arch(argc: usize, argv: *const usize)
{
    loop {
        unsafe {
            let argv = slice::from_raw_parts(argv, argc);
            // asm!("xorq %rax, %rax" : : : "rax" : );
            // asm!("inc %rax" : : : "rax" : );
            // asm!("mov rax, 0xAFAF" : : : "rax" : "intel", "volatile");
            asm!("mov rax, $0
                  mov rbx, $1
                  mov rcx, 0xFFF
                  hlt
                 "
                 :
                 : "r"(argc), "r"(argv[0])
                 : "rax", "rbx", "rcx"
                 : "intel", "volatile"
                );
            // asm!("hlt");
        }
    }
}
