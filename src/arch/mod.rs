//! This module contains all codes depending on the architecture to abstract these codes.

#[cfg(target_arch = "arm")]
mod arm11;

#[cfg(target_arch = "arm")]
pub fn init_arch(_: &[usize])
{
    arm11::init();
}



#[cfg(target_arch = "x86")]
mod x86_32;

#[cfg(target_arch = "x86")]
pub fn init_arch(argv: &[usize])
{
    x86_32::init(argv);
}



#[cfg(target_arch = "x86_64")]
mod x86_64;

#[cfg(target_arch = "x86_64")]
pub fn init_arch(argv: &[usize])
{
    x86_64::init(argv);
}
