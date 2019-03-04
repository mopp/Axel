//! This module contains all codes depending on the architecture to abstract these codes.
use crate::graphic;
use crate::memory::address::VirtualAddress;
use crate::memory::Error as MemoryError;
use failure::Fail;

// TODO: introduce ErrorKind.
#[derive(Fail, Debug)]
pub enum Error {
    #[fail(display = "No memory map is in multiboot_info")]
    NoMemoryMap,
    #[fail(display = "Memory initialization faild: {}", _0)]
    MemoryInitializationFailed(MemoryError),
}

impl From<MemoryError> for Error {
    fn from(e: MemoryError) -> Error {
        Error::MemoryInitializationFailed(e)
    }
}

/// Common interfaces between architectures.
pub trait Initialize {
    fn init(argv: &[VirtualAddress]) -> Result<(), Error>;
    fn obtain_kernel_console() -> Option<graphic::CharacterDisplay<'static>>;
}

macro_rules! with_arch {
    ($arch: ident) => {
        mod $arch;
        pub use self::$arch::Initializer;
        pub use self::$arch::Thread;
    };
}

#[cfg(target_arch = "arm11")]
with_arch!(arm11);

#[cfg(target_arch = "x86_32")]
with_arch!(x86_32);

#[cfg(target_arch = "x86_64")]
with_arch!(x86_64);
