#[macro_export]
macro_rules! print {
    ($($args:tt)*) => {
        {
            use context;
            let ref mut default_output = *context::GLOBAL_CONTEXT.default_output.lock();
            if let Some(ref mut default_output) = *default_output {
                use core::fmt::Write;
                write!(default_output, $($args)*).unwrap();
            }
        }
    }
}

#[macro_export]
macro_rules! println {
    ($fmt:expr)              => (print!(concat!($fmt, "\n")));
    ($fmt:expr, $($arg:tt)*) => (print!(concat!($fmt, "\n"), $($arg)*));
}
