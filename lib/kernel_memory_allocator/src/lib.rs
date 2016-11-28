#![feature(allocator)]
#![allocator]
#![no_std]


#[no_mangle]
pub extern fn __rust_allocate(size: usize, _align: usize) -> *mut u8
{
    panic!("__rust_allocate");
    unsafe { 0 as *mut u8 }
}


#[no_mangle]
pub extern fn __rust_deallocate(ptr: *mut u8, _old_size: usize, _align: usize)
{
    panic!("__rust_deallocate");
    unsafe {}
}


#[no_mangle]
pub extern fn __rust_reallocate(ptr: *mut u8, _old_size: usize, size: usize, _align: usize) -> *mut u8
{
    panic!("__rust_reallocate");
    unsafe { 0 as *mut u8 }
}


#[no_mangle]
pub extern fn __rust_reallocate_inplace(_ptr: *mut u8, old_size: usize, _size: usize, _align: usize) -> usize
{
    panic!("__rust_reallocate_inplace");
    0
}


#[no_mangle]
pub extern fn __rust_usable_size(size: usize, _align: usize) -> usize
{
    panic!("__rust_usable_size");
    size
}
