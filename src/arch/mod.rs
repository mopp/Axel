//! This module contains all codes depending on the architecture to abstract these codes.


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
            // asm!("xorq %rax, %rax" : : : "rax" : );
            // asm!("inc %rax" : : : "rax" : );
            asm!("mov rax, 0xAFAF" : : : "rax" : "intel", "volatile");
            // asm!("movq %rbx, 0x111111" : : : "rbx" : );
            // asm!("movq %rcx, 0xFFAAFFAA" : : : "rcx" : );
            asm!("hlt");
        }
    }
}
