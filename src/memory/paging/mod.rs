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
mod table;
mod page;

pub use self::page::Page;
use self::entry::PageEntryFlags;
use self::table::ActivePageTable;
use super::address::address_of;
use super::buddy_system::BuddyAllocator;
use super::frame::{Frame, FrameAdapter};
use super::Error;
use intrusive_collections::UnsafeRef;
use intrusive_collections::IntrusivePointer;

pub fn init(mut bman: BuddyAllocator<FrameAdapter, Frame>) -> Result<(), Error> {
    let active_page_table = unsafe { ActivePageTable::new() };
    let level4_table = active_page_table.level4_page_table();
    println!("level4_table - {:x}", address_of(level4_table));
    level4_table
        .next_level_table(511)
        .and_then(|level3_table| {
            println!("level3_table - {:x}", address_of(level3_table));
            level3_table.next_level_table(511)
        })
        .and_then(|level2_table| {
            println!("level2_table - {:x}", address_of(level2_table));
            level2_table.next_level_table(511)
        })
        .map(|level1_table| {
            println!("level1_table - {:x}", address_of(level1_table));
            for i in 0..512 {
                if level1_table[i].flags().contains(PageEntryFlags::Present) {
                    println!("  entry({}) - {:x}", i, level1_table[i]);
                }
            }
        });

    let frame: UnsafeRef<Frame> = bman.allocate(1).unwrap();
    println!("Allocate frame1:");
    println!("  {:?}", frame.clone().into_raw());
    println!("  {:?}", frame);

    let frame = bman.allocate(1).unwrap();
    println!("Allocate frame2:");
    println!("  {:?}", frame.clone().into_raw());
    println!("  {:?}", frame);

    // TODO: create new kernel page table.
    let ptr = frame.clone().into_raw() as usize;
    let tmp = active_page_table.get_physical_address(ptr);
    println!("{:x} -> 0x{:x}", ptr, tmp.unwrap());

    let ptr = (&frame as *const _) as usize;
    let tmp = active_page_table.get_physical_address(ptr);
    println!("{:x} -> 0x{:x}", ptr, tmp.unwrap());

    Ok(())
}
