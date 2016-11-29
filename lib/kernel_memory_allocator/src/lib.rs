#![allow(dead_code)]
#![allow(unused_variables)]

#![feature(allocator)]
#![allocator]
#![no_std]


#[no_mangle]
pub extern fn __rust_allocate(size: usize, alignment: usize) -> *mut u8
{
    panic!("__rust_allocate");
}


#[no_mangle]
pub extern fn __rust_deallocate(ptr: *mut u8, old_size: usize, alignment: usize)
{
    panic!("__rust_deallocate");
}


#[no_mangle]
pub extern fn __rust_reallocate(ptr: *mut u8, old_size: usize, size: usize, alignment: usize) -> *mut u8
{
    panic!("__rust_reallocate");
}


#[no_mangle]
pub extern fn __rust_reallocate_inplace(_ptr: *mut u8, old_size: usize, size: usize, alignment: usize) -> usize
{
    panic!("__rust_reallocate_inplace");
}


#[no_mangle]
pub extern fn __rust_usable_size(size: usize, alignment: usize) -> usize
{
    panic!("__rust_usable_size");
}
