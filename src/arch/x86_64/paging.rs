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


use core::ops::{Index, IndexMut};
use core::marker::PhantomData;


bitflags! {
    pub flags EntryFlags: usize {
        const PRESENT         = 1 << 0,
        const WRITABLE        = 1 << 1,
        const USER_ACCESSIBLE = 1 << 2,
        const WRITE_THROUGH   = 1 << 3,
        const CACHE_DISABLE   = 1 << 4,
        const ACCESSED        = 1 << 5,
        const DIRTY           = 1 << 6,
        const HUGE_PAGE       = 1 << 7,
        const GLOBAL          = 1 << 8,
        const NO_EXECUTE      = 1 << 63,
    }
}


/// A page entry.
struct Entry(usize);


impl Entry {
    /// Return `EntryFlags`.
    fn flags(&self) -> EntryFlags
    {
        EntryFlags::from_bits_truncate(self.0)
    }


    /// Clear the all flags (set the all flags zero).
    fn clear_all(&mut self)
    {
        self.0 = 0;
    }
}


/// Signature enum for Level1 page table.
enum Level1 {}
/// Signature enum for Level2 page table.
enum Level2 {}
/// Signature enum for Level3 page table.
enum Level3 {}
/// Signature enum for Level4 page table.
enum Level4 {}


/// Signature trait for manipulating enries in the `Table<T>` struct.
trait Level {}

impl Level for Level4 {}
impl Level for Level3 {}
impl Level for Level2 {}
impl Level for Level1 {}

/// It provides trait bound for generating next level page table struct.
trait HierarchicalLevel: Level {
    type NextLevel: Level;
}

impl HierarchicalLevel for Level4 { type NextLevel = Level3; }
impl HierarchicalLevel for Level3 { type NextLevel = Level2; }
impl HierarchicalLevel for Level2 { type NextLevel = Level1; }


const MAX_ENTRY_COUNT: usize = 512;


/// A page table.
struct Table<T> where T: Level {
    entries: [Entry; MAX_ENTRY_COUNT],
    level: PhantomData<T>,
}


impl<T> Table<T> where T: Level {
    fn clear_all_entries(&mut self)
    {
        for entry in self.entries.iter_mut() {
            entry.clear_all();
        }
    }
}

impl<T> Table<T> where T: HierarchicalLevel {
    fn next_level_table_address(&self, index: usize) -> Option<usize> {
        let entry_flags = self[index].flags();
        if (entry_flags.contains(PRESENT) == true) && (entry_flags.contains(HUGE_PAGE) == false) {
            Some((address_of!(self) << 9) | (index << 12))
        } else {
            None
        }
    }


    pub fn next_level_table<'a>(&'a self, index: usize) -> Option<&'a Table<T::NextLevel>>
    {
        self.next_level_table_address(index).map(|address| unsafe { &*(address as *const _) })
    }


    pub fn next_level_table_mut<'a>(&'a self, index: usize) -> Option<&'a mut Table<T::NextLevel>>
    {
        self.next_level_table_address(index).map(|address| unsafe { &mut *(address as *mut _) })
    }
}


impl<T> Index<usize> for Table<T> where T: Level {
    type Output = Entry;

    fn index(&self, index: usize) -> &Entry
    {
        &self.entries[index]
    }
}


impl<T> IndexMut<usize> for Table<T> where T: Level {
    fn index_mut(&mut self, index: usize) -> &mut Entry
    {
        &mut self.entries[index]
    }
}


const LEVEL4_PAGE_TABLE: *mut Table<Level4> = 0xffff_ffff_ffff_f000 as *mut _;



#[cfg(test)]
mod test {
    // use super::{LEVEL4_PAGE_TABLE, Table};

    #[test]
    fn test_invalid_page_table()
    {
    }
}
