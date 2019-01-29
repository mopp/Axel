use super::entry::{PageEntry, PageEntryFlags};
use super::{Page, PageIndex};
use crate::memory::address::{PhysicalAddress, VirtualAddress};
use crate::memory::frame::Frame;
use crate::memory::FrameAllocator;
use core::marker::PhantomData;
use core::ops::{Index, IndexMut};
use core::ptr::Unique;
use x86_64::instructions::tlb;
use x86_64::registers;
use x86_64::structures::paging::PhysFrame;

#[derive(Fail, Debug)]
pub enum Error {
    #[fail(display = "No page table")]
    NoPageTable,
    #[fail(display = "Already mapped page")]
    AlreadyMapped,
    #[fail(display = "There is no page table entry")]
    NoEntry,
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

    pub fn find_free_entry_index(&self) -> Option<usize> {
        // TODO: implement Iter.
        for i in 0..512 {
            if self[i].flags().contains(PageEntryFlags::Present) == false {
                return Some(i);
            }
        }

        None
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

    // pub fn next_level_table<'a>(&'a self, index: usize) -> Option<&'a Table<T::NextLevel>> {
    //     self.next_level_table_address(index).map(|address| unsafe { &*(address as *const _) })
    // }

    pub fn next_level_table_mut<'a>(&'a self, index: usize) -> Option<&'a mut Table<T::NextLevel>> {
        self.next_level_table_address(index).map(|address| unsafe { &mut *(address as *mut _) })
    }

    pub fn next_level_table_create_mut(&mut self, index: usize, allocator: &mut FrameAllocator) -> Option<&mut Table<T::NextLevel>> {
        // TODO; refactor
        if self.next_level_table_mut(index).is_some() {
            return self.next_level_table_mut(index);
        }

        if let Some(frame) = allocator.alloc_one() {
            self[index].set_frame_addr(frame.address());
            if let Some(table) = self.next_level_table_mut(index) {
                table.clear_all_entries();
                Some(table)
            } else {
                debug_assert!(false, "error");
                None
            }
        } else {
            // No memory.
            None
        }
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

    // pub fn level4_page_table(&self) -> &Table<Level4> {
    //     unsafe { self.level4_page_table.as_ref() }
    // }

    pub fn level4_page_table_mut(&mut self) -> &mut Table<Level4> {
        unsafe { self.level4_page_table.as_mut() }
    }

    // pub fn get_physical_address(&self, addr: VirtualAddress) -> Option<PhysicalAddress> {
    //     debug_assert!(addr <= 0x0000_8000_0000_0000 || 0xffff_8000_0000_0000 <= addr, "0x{:x} is invalid address", addr);
    //
    //     let level3_table = self.level4_page_table().next_level_table(addr.level4_index());
    //
    //     level3_table
    //         .and_then(|t| t.next_level_table(addr.level3_index()))
    //         .and_then(|t| t.next_level_table(addr.level2_index()))
    //         .and_then(|t| t[addr.level1_index()].get_frame_addr())
    //         .or_else(|| {
    //             level3_table.and_then(|t3| {
    //                 let entry = &t3[addr.level3_index()];
    //                 // Return address here if the page size is 1GB.
    //                 if entry.flags().contains(PageEntryFlags::HugePage) {
    //                     return entry.get_frame_addr();
    //                 }
    //
    //                 if let Some(t2) = t3.next_level_table(addr.level3_index()) {
    //                     let entry = &t2[addr.level2_index()];
    //                     if entry.flags().contains(PageEntryFlags::HugePage) {
    //                         return entry.get_frame_addr();
    //                     }
    //                 }
    //
    //                 None
    //             })
    //         })
    // }

    pub fn map(&mut self, page: Page, frame: Frame, allocator: &mut FrameAllocator) -> Result<(), Error> {
        let page_addr = page.address();
        let frame_addr = frame.address();

        let table = self
            .level4_page_table_mut()
            .next_level_table_create_mut(page_addr.level4_index(), allocator)
            .and_then(|t| t.next_level_table_create_mut(page_addr.level3_index(), allocator))
            .and_then(|t| t.next_level_table_create_mut(page_addr.level2_index(), allocator));

        if let Some(table) = table {
            let entry = &mut table[page_addr.level1_index()];
            if entry.flags().contains(PageEntryFlags::Present) == false {
                entry.set_frame_addr(frame_addr);
                Ok(())
            } else {
                Err(Error::AlreadyMapped)
            }
        } else {
            Err(Error::NoPageTable)
        }
    }

    // TODO: consider suitable name.
    pub fn map_fitting(&mut self, v_range: (VirtualAddress, VirtualAddress), p_range: (PhysicalAddress, PhysicalAddress), allocator: &mut FrameAllocator) -> Result<(), Error> {
        let (v_begin, v_end) = v_range;
        let (p_begin, p_end) = p_range;
        debug_assert_eq!(v_end - v_begin, p_end - p_begin);

        let size = v_end - v_begin;
        let count_2mb = size / (2 * 1024 * 1024);
        let count_4kb = (size - count_2mb * 2 * 1024 * 1024) / 4096;

        // FIXME: Revert mappings if mapping fails in these process.
        let mut offset = 0;

        if 0 < count_2mb {
            unimplemented!("TODO: Implement mapping for 2MB pages")
        }

        if 0 < count_4kb {
            // FIXME: Re-implement them more effectively.
            for _ in 0..count_4kb {
                let page = Page::from_address(v_begin + offset);
                let frame = Frame::from_address(p_begin + offset);

                self.map(page, frame, allocator)?;

                offset += 4096;
            }
        }

        Ok(())
    }

    pub fn find_empty_page(&mut self, allocator: &mut FrameAllocator) -> Option<Page> {
        let table = self.level4_page_table_mut();

        let mut i4: usize = 0;
        let mut i3: usize = 0;
        let mut i2: usize = 0;
        table
            .find_free_entry_index()
            .and_then(|i| {
                i4 = i;
                table.next_level_table_create_mut(i, allocator)
            })
            .and_then(|table| {
                if let Some(i) = table.find_free_entry_index() {
                    i3 = i;
                    table.next_level_table_create_mut(i, allocator)
                } else {
                    None
                }
            })
            .and_then(|table| {
                if let Some(i) = table.find_free_entry_index() {
                    i2 = i;
                    table.next_level_table_create_mut(i, allocator)
                } else {
                    None
                }
            })
            .and_then(|table| {
                if let Some(i1) = table.find_free_entry_index() {
                    let i = i4 * 512 * 512 * 512 + i3 * 512 * 512 + i2 * 512 + i1;
                    Some(Page::from_number(i))
                } else {
                    None
                }
            })
    }

    pub fn with(&mut self, inactive_page_table: &mut InActivePageTable, allocator: &mut FrameAllocator, f: (impl Fn(&mut ActivePageTable, &mut FrameAllocator) -> Result<(), Error>)) -> Result<(), Error> {
        // Keep the current active page table entry to restore it.
        let addr = registers::control::Cr3::read().0.start_address().as_u64() as usize;
        let original_table_frame = Frame::from_address(addr);
        let page = self.find_empty_page(allocator).ok_or(Error::NoEntry)?;
        self.map(page.clone(), original_table_frame, allocator)?;
        let original_table = unsafe { &mut *(page.address() as *mut Table<Level1>) };

        // Override the recursive mapping.
        let entry = &mut self.level4_page_table_mut()[511];
        entry.set_frame_addr(inactive_page_table.level4_page_table.address());
        entry.set_flags(PageEntryFlags::Writable);
        tlb::flush_all();

        let result = f(self, allocator);

        // Restore the active page table entry.
        let entry = &mut original_table[511];
        entry.set_frame_addr(addr);
        entry.set_flags(PageEntryFlags::Writable);

        // Restore the active page table entry.
        let entry = &mut self.level4_page_table_mut()[511];
        entry.set_frame_addr(addr);
        entry.set_flags(PageEntryFlags::Writable);

        tlb::flush_all();

        result
    }

    pub fn switch(&mut self, new_table: InActivePageTable) -> InActivePageTable {
        let old_table = InActivePageTable {
            level4_page_table: Frame::from_address(registers::control::Cr3::read().0.start_address().as_u64() as usize),
        };

        let addr = x86_64::PhysAddr::new(new_table.level4_page_table.address() as u64);
        let frame = PhysFrame::containing_address(addr);
        let flags = registers::control::Cr3Flags::empty();

        unsafe {
            registers::control::Cr3::write(frame, flags);
        }

        old_table
    }
}

pub struct InActivePageTable {
    level4_page_table: Frame,
}

impl InActivePageTable {
    pub fn new(active_page_table: &mut ActivePageTable, allocator: &mut FrameAllocator) -> Option<InActivePageTable> {
        allocator.alloc_one().map(|frame| {
            let p = active_page_table.find_empty_page(allocator).unwrap();
            active_page_table.map(p.clone(), frame.clone(), allocator).unwrap();
            let table = unsafe { &mut *(p.address() as *mut Table<Level1>) };
            table.clear_all_entries();

            // set recursive page mapping.
            let entry = &mut table[511];
            entry.set_frame_addr(frame.address());
            entry.set_flags(PageEntryFlags::Writable);

            InActivePageTable { level4_page_table: frame }
        })
    }
}
