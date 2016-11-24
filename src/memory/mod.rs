pub mod region;


#[macro_export]
macro_rules! addr_of_var {
    ($x: expr) => (addr_of_ref!(&$x));
}


#[macro_export]
macro_rules! addr_of_ref {
    ($x: expr) => (($x as *const _) as usize);
}


#[macro_export]
macro_rules! align_up {
    ($align: expr, $n: expr) => {
        {
            let n = $n;
            let mask = $align - 1;
            (n + mask) & (!mask)
        }
    };
}


#[macro_export]
macro_rules! align_down {
    ($align: expr, $n: expr) => {
        {
            let n = $n;
            let mask = $align - 1;
            n & (!mask)
        }
    };
}


// These variable are defined in the linker script and their value do NOT have any meanings.
// Their addressees have meanings.
extern {
    static KERNEL_ADDR_BSS_BEGIN: usize;
    static KERNEL_SIZE_BSS: usize;
}


pub fn clean_bss_section()
{
    unsafe {
        let begin = addr_of_var!(KERNEL_ADDR_BSS_BEGIN) as *mut u8;
        let size  = addr_of_var!(KERNEL_SIZE_BSS) as i32;

        use rlibc;
        rlibc::memset(begin, size, 0x00);
    }
}
