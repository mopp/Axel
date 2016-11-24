//! This module contains all codes depending on the architecture to abstract these codes.

macro_rules! generate {
    ($x:ident) => {
        mod $x;
        pub fn init_arch(argv: &[usize])
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
