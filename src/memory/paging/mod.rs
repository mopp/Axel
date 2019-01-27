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

pub use self::page::Page;
pub use self::page_index::PageIndex;

use self::table::{ActivePageTable, InActivePageTable};
use super::buddy_system::BuddyAllocator;
use super::Error;
use super::{Frame, FrameAdapter, FrameAllocator};

pub fn init(mut bman: BuddyAllocator<FrameAdapter, Frame>) -> Result<(), Error> {
    let mut active_page_table = unsafe { ActivePageTable::new() };

    runtime_test(&mut active_page_table, &mut bman)?;

    // TODO: create new kernel page table.
    let inactive_page_table = InActivePageTable::new(&mut active_page_table, &mut bman).ok_or(Error::NoUsableMemory)?;
    active_page_table.with(inactive_page_table, &mut bman, |_, _| ())?;

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

    // FIXME: unmap it.

    Ok(())
}
