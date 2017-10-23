//! This module contains all codes depending on the architecture to abstract these codes.

use memory::address::VirtualAddress;

macro_rules! generate {
    ($x:ident) => {
        mod $x;
        pub use self::$x::obtain_kernel_console;
        pub fn init_arch(argv: &[VirtualAddress])
        {
            $x::init(argv);
        }
    }
}


// FIXME
#[cfg(target_arch = "arm11")]
generate!(arm11);

// FIXME
#[cfg(target_arch = "x86_32")]
generate!(x86_32);


#[cfg(target_arch = "x86_64")]
generate!(x86_64);
