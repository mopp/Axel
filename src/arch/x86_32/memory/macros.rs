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
