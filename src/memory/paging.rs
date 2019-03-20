//! The name conversions are defined because Intel's names are confusable I think.
//! Name conversions:
//!     Frame              := Physical memory block
//!     Page               := Virtual memory block
//!     Level4 Table/Entry := PML4E, Page Map Level 4 Table/Entry
//!     Level3 Table/Entry := PDPTE, Page Directory Pointer Table/Entry
//!     Level2 Table/Entry := PDE, Page Directory Table/Entry
//!     Level1 Table/Entry := PTE, Page Table/Entry
//!
//! In x86_64 system, the virtual address is composed of four parts (in 4KB mapping):
//! Note that the virtual address is called linear address in the intel manual.
//!      9bit(Index of level 4)
//!      9bit(Index of level 3)
//!      9bit(Index of level 2)
//!      9bit(Index of level 1)
//!     12bit(Physical address offset)
//!     9 + 9 + 9 + 9 + 12 = 48
//!      ------------------------------------------------
//!      | Level4 | Level3 | Level2 | Level1 |  Offset  |
//!      ------------------------------------------------
//!     47      39 38    30 29    22 20    12 11        0
//!
//! Each index is represented by 9bit.
//! This means the number of the entries in each table is 512 (2^9) respectively.
//! Then, the size of an entry is 8bytes.
//! So, The size of a table is (8 * 512 = 4096 = 4KB) and it corresponds the minimum page size.
//!
//! For more information, please refer 4.5 IA-32E PAGING in the intel manual.
//!
//! Reference:
//!     [Page Tables](http://os.phil-opp.com/modifying-page-tables.html)

mod entry;
mod page;
mod page_index;
pub mod table;

use super::address;
use super::address::*;
use super::buddy_system::BuddyAllocator;
use super::frame;
use super::Error;
use super::{Frame, FrameAdapter, FrameAllocator};
use crate::bytes::Bytes;
pub use page::Page;
pub use page_index::PageIndex;
pub use table::{ActivePageTable, InActivePageTable, ACTIVE_PAGE_TABLE};

pub struct IdenticalReMapRequest(PhysicalAddress, usize);

impl IdenticalReMapRequest {
    pub fn new(addr: PhysicalAddress, count: usize) -> IdenticalReMapRequest {
        IdenticalReMapRequest(addr, count)
    }
}

pub fn init(remap_requests: &[IdenticalReMapRequest], bman: &mut BuddyAllocator<FrameAdapter, Frame>) -> Result<(), Error> {
    let mut active_page_table = ACTIVE_PAGE_TABLE.lock();

    runtime_test(&mut active_page_table, bman)?;

    let kernel_begin = address::kernel_addr_begin_virtual();
    let kernel_end = address::kernel_addr_end_physical().to_virtual_addr();
    println!("Kernel virtual address: 0x{:x} - 0x{:x}, {}KB", kernel_begin, kernel_end, (kernel_end - kernel_begin).kb());
    debug_assert!(kernel_begin < kernel_end);
    debug_assert_eq!(0, kernel_begin & 0xfff);
    debug_assert_eq!(0, kernel_end & 0xfff);

    // Create new kernel page table.
    let mut inactive_page_table = InActivePageTable::new(&mut active_page_table, bman).ok_or(Error::NoUsableMemory)?;
    active_page_table.with(&mut inactive_page_table, bman, |table, allocator| {
        let v_range = (kernel_begin, kernel_end);
        let p_range = (address::kernel_addr_begin_physical(), address::kernel_addr_end_physical());

        for IdenticalReMapRequest(addr, count) in remap_requests {
            // debug_assert!(*addr < kernel_begin || kernel_end <= *addr);
            // debug_assert!((addr + frame::SIZE * count) < kernel_begin || kernel_end <= (addr + frame::SIZE * count));

            for i in 0..*count {
                let addr = addr + i * frame::SIZE;
                let page = Page::from_address(addr.to_virtual_addr());
                let frame = Frame::from_address(addr);
                table.map(page, frame, allocator)?;
            }
        }

        table.map_fitting(v_range, p_range, allocator)
    })?;

    // Switch the active page table to new one.
    let _old_table = active_page_table.switch(inactive_page_table);
    // TODO: Free the old table.

    Ok(())
}

#[inline(always)]
pub fn runtime_test(active_page_table: &mut ActivePageTable, allocator: &mut FrameAllocator) -> Result<(), Error> {
    // Runtime test.
    let frame = allocator.alloc_one().ok_or(Error::NoUsableMemory)?;

    // FIXME: use more proper address.
    let mut page = Page::from_address(0x200000);
    active_page_table.map(page.clone(), frame, allocator)?;

    // It will not cause page fault.
    unsafe {
        for i in page.to_slice_mut() {
            *i = 1;
        }
    }

    active_page_table.unmap(page, allocator).map_err(Into::into)
}
