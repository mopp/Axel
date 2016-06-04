#[macro_export]
macro_rules! print {
    ($($args:tt)*) => {
        {
            unsafe {
                use axel_context;
                if let Some(ref mut kernel_output_device) = axel_context::AXEL_CONTEXT.kernel_output_device {
                    use core::fmt::Write;
                    write!(kernel_output_device, $($args)*).unwrap();
                }
            }
        }
    }
}


#[macro_export]
macro_rules! println {
    ($fmt:expr)              => (print!(concat!($fmt, "\n")));
    ($fmt:expr, $($arg:tt)*) => (print!(concat!($fmt, "\n"), $($arg)*));
}
