use super::entry::{PageEntry, PageEntryFlags};
use core::marker::PhantomData;
use core::ops::{Index, IndexMut};
use core::ptr::Unique;
use memory::address::{VirtualAddress, PhysicalAddress};

trait PageIndex {
    fn level4_index(self) -> usize;
    fn level3_index(self) -> usize;
    fn level2_index(self) -> usize;
    fn level1_index(self) -> usize;
    fn offset(self) -> usize;
}

impl PageIndex for VirtualAddress {
    fn level4_index(self) -> usize {
        (self >> 39) & 0o777
    }

    fn level3_index(self) -> usize {
        (self >> 30) & 0o777
    }

    fn level2_index(self) -> usize {
        (self >> 21) & 0o777
    }

    fn level1_index(self) -> usize {
        (self >> 12) & 0o777
    }

    fn offset(self) -> usize {
        self & 0xFFF
    }
}


/// Signature trait for manipulating enries in the `Table<T>` struct.
pub trait Level {}

/// It provides trait bound for generating next level page table struct.
pub trait HierarchicalLevel: Level {
    type NextLevel: Level;
}

/// Signature struct for Level1 page table.
pub struct Level1;

/// Signature struct for Level2 page table.
pub struct Level2;

/// Signature struct for Level3 page table.
pub struct Level3;

/// Signature struct for Level4 page table.
pub struct Level4;

/// A page table.
pub struct Table<T>
where
    T: Level,
{
    entries: [PageEntry; MAX_ENTRY_COUNT],
    level: PhantomData<T>,
}

impl Level for Level4 {}
impl Level for Level3 {}
impl Level for Level2 {}
impl Level for Level1 {}

impl HierarchicalLevel for Level4 {
    type NextLevel = Level3;
}
impl HierarchicalLevel for Level3 {
    type NextLevel = Level2;
}
impl HierarchicalLevel for Level2 {
    type NextLevel = Level1;
}

const LEVEL4_PAGE_TABLE: *mut Table<Level4> = 0xffff_ffff_ffff_f000 as *mut _;
const MAX_ENTRY_COUNT: usize = 512;

impl<T> Table<T>
where
    T: Level,
{
    fn clear_all_entries(&mut self) {
        for entry in self.entries.iter_mut() {
            entry.clear_all();
        }
    }
}

impl<T> Table<T>
where
    T: HierarchicalLevel,
{
    fn next_level_table_address(&self, index: usize) -> Option<usize> {
        let entry_flags = self[index].flags();
        if (entry_flags.contains(PageEntryFlags::Present) == true) && (entry_flags.contains(PageEntryFlags::HugePage) == false) {
            Some((((self as *const _) as usize) << 9) | (index << 12))
        } else {
            None
        }
    }

    pub fn next_level_table<'a>(&'a self, index: usize) -> Option<&'a Table<T::NextLevel>> {
        self.next_level_table_address(index).map(|address| unsafe { &*(address as *const _) })
    }

    pub fn next_level_table_mut<'a>(&'a self, index: usize) -> Option<&'a mut Table<T::NextLevel>> {
        self.next_level_table_address(index).map(|address| unsafe { &mut *(address as *mut _) })
    }
}

impl<T> Index<usize> for Table<T>
where
    T: Level,
{
    type Output = PageEntry;

    fn index(&self, index: usize) -> &PageEntry {
        &self.entries[index]
    }
}

impl<T> IndexMut<usize> for Table<T>
where
    T: Level,
{
    fn index_mut(&mut self, index: usize) -> &mut PageEntry {
        &mut self.entries[index]
    }
}

pub struct ActivePageTable {
    level4_page_table: Unique<Table<Level4>>,
}

impl ActivePageTable {
    pub unsafe fn new() -> ActivePageTable {
        ActivePageTable {
            level4_page_table: Unique::new_unchecked(LEVEL4_PAGE_TABLE),
        }
    }

    pub fn level4_page_table(&self) -> &Table<Level4> {
        unsafe { self.level4_page_table.as_ref() }
    }

    pub fn level4_page_table_mut(&mut self) -> &mut Table<Level4> {
        unsafe { self.level4_page_table.as_mut() }
    }

    pub fn get_physical_address(&self, addr: VirtualAddress) -> Option<PhysicalAddress> {
        // Support huge page.
        self.level4_page_table()
            .next_level_table(addr.level4_index())
            .and_then(|t|  {
                t.next_level_table(addr.level3_index())
            })
            .and_then(|t|  {
                t.next_level_table(addr.level2_index())
            })
            .and_then(|t|  {
                t[addr.level1_index()].get_frame_addr()
            })
            .or_else(|| None)
    }
}
