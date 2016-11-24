#[macro_export]
macro_rules! print {
    ($($args:tt)*) => {
        {
            use axel_context;
            let ref context = *axel_context::GLOBAL_CONTEXT;
            let mut kernel_output_device = context.kernel_output_device.lock();
            if let Some(ref mut kernel_output_device) = *kernel_output_device {
                use core::fmt::Write;
                write!(kernel_output_device, $($args)*).unwrap();
            }
        }
    }
}


#[macro_export]
macro_rules! println {
    ($fmt:expr)              => (print!(concat!($fmt, "\n")));
    ($fmt:expr, $($arg:tt)*) => (print!(concat!($fmt, "\n"), $($arg)*));
}
