//! This module contains all codes depending on the architecture to abstract these codes.
use graphic;
use memory::address::VirtualAddress;

/// Common interfaces between architectures.
pub trait Initialize {
    fn init(argv: &[VirtualAddress]) -> Result<(), &'static str>;
    fn obtain_kernel_console() -> Option<graphic::CharacterDisplay<'static>>;
}

macro_rules! with_arch {
    ($arch: ident) => {
        mod $arch;
        pub use self::$arch::Initializer;
    };
}

#[cfg(target_arch = "arm11")]
with_arch!(arm11);

#[cfg(target_arch = "x86_32")]
with_arch!(x86_32);

#[cfg(target_arch = "x86_64")]
with_arch!(x86_64);
