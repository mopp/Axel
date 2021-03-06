use super::entry::{PageEntry, PageEntryFlags};
use super::page;
use super::{Page, PageIndex};
use crate::memory::address::{PhysicalAddress, VirtualAddress};
use crate::memory::frame::{self, Frame};
use crate::memory::FrameAllocator;
use core::marker::PhantomData;
use core::ops::{Index, IndexMut};
use core::ptr::Unique;
use failure::Fail;
use spin::Mutex;
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
        self.find_free_entry_index_with(1)
    }

    /// Find begin index of N free pages.
    pub fn find_free_entry_index_with(&self, count: usize) -> Option<usize> {
        let mut index = None;
        let mut c = count;

        // TODO: implement Iter.
        // Start from 1 to avoid using null.
        for i in 1..512 {
            if self[i].flags().contains(PageEntryFlags::PRESENT) == false {
                if index.is_none() {
                    index = Some(i)
                }

                c -= 1;

                if c == 0 {
                    return index;
                }
            } else {
                c = count
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
        let flags = self[index].flags();
        if flags.contains(PageEntryFlags::PRESENT) && !flags.contains(PageEntryFlags::HUGE_PAGE) {
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

    pub fn next_level_table_create_mut(&mut self, index: usize, allocator: &mut dyn FrameAllocator) -> Option<&mut Table<T::NextLevel>> {
        // TODO; refactor
        if self.next_level_table_mut(index).is_some() {
            return self.next_level_table_mut(index);
        }

        if let Some(frame) = allocator.alloc_one() {
            self[index].set_frame_addr(frame.address());
            if let Some(table) = self.next_level_table_mut(index) {
                table.clear_all_entries();

                let entry = &mut table[511];
                entry.set_frame_addr(frame.address());
                entry.set_flags(PageEntryFlags::WRITABLE);

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

pub static ACTIVE_PAGE_TABLE: Mutex<ActivePageTable> = Mutex::new(ActivePageTable {
    level4_page_table: unsafe { Unique::new_unchecked(LEVEL4_PAGE_TABLE) },
});

pub struct ActivePageTable {
    level4_page_table: Unique<Table<Level4>>,
}

impl ActivePageTable {
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

    /// FIXME: support N:N mapping.
    pub fn map(&mut self, page: Page, frame: Frame, allocator: &mut dyn FrameAllocator) -> Result<(), Error> {
        let page_addr = page.address();
        let frame_addr = frame.address();

        let table = self
            .level4_page_table_mut()
            .next_level_table_create_mut(page_addr.level4_index(), allocator)
            .and_then(|t| t.next_level_table_create_mut(page_addr.level3_index(), allocator))
            .and_then(|t| t.next_level_table_create_mut(page_addr.level2_index(), allocator));

        if let Some(table) = table {
            let entry = &mut table[page_addr.level1_index()];
            if entry.flags().contains(PageEntryFlags::PRESENT) == false {
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
    pub fn map_fitting(&mut self, v_range: (VirtualAddress, VirtualAddress), p_range: (PhysicalAddress, PhysicalAddress), allocator: &mut dyn FrameAllocator) -> Result<(), Error> {
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

    pub fn auto_continuous_map(&mut self, frame: &Frame, allocator: &mut dyn FrameAllocator) -> Option<(VirtualAddress, VirtualAddress)> {
        let frame_count = 1 << frame.order();
        let size = frame_count * frame::SIZE;
        let page = self.find_empty_pages(frame_count, allocator)?;

        for i in 0..frame_count {
            let frame = Frame::from_address(frame.address() + i * frame::SIZE);
            let page = Page::from_address(page.address() + i * page::SIZE);

            if self.map(page, frame, allocator).is_err() {
                panic!("FIXME")
            }
        }

        let addr = page.address();
        Some((addr, addr + size))
    }

    pub fn unmap(&mut self, page: Page, allocator: &mut dyn FrameAllocator) -> Result<(), Error> {
        let addr = page.address();

        // FIXME: Support huge page.
        let table = self
            .level4_page_table_mut()
            .next_level_table_mut(addr.level4_index())
            .and_then(|t| t.next_level_table_mut(addr.level3_index()))
            .and_then(|t| t.next_level_table_mut(addr.level2_index()))
            .ok_or(Error::NoEntry)?;

        let entry = &mut table[addr.level1_index()];
        let addr = entry.get_frame_addr().ok_or(Error::NoEntry)?;
        allocator.free(Frame::from_address(addr));

        entry.clear_all();
        Ok(())
    }

    pub fn find_empty_page(&mut self, allocator: &mut dyn FrameAllocator) -> Option<Page> {
        self.find_empty_pages(1, allocator)
    }

    // FIXME: support large page.
    pub fn find_empty_pages(&mut self, count: usize, allocator: &mut dyn FrameAllocator) -> Option<Page> {
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
                if let Some(i1) = table.find_free_entry_index_with(count) {
                    let i = i4 * 512 * 512 * 512 + i3 * 512 * 512 + i2 * 512 + i1;
                    Some(Page::from_number(i))
                } else {
                    None
                }
            })
    }

    pub fn with(&mut self, inactive_page_table: &mut InActivePageTable, allocator: &mut dyn FrameAllocator, f: (impl Fn(&mut ActivePageTable, &mut dyn FrameAllocator) -> Result<(), Error>)) -> Result<(), Error> {
        // Keep the current active page table entry to restore it.
        let addr = registers::control::Cr3::read().0.start_address().as_u64() as usize;
        let original_table_frame = Frame::from_address(addr);
        let page = self.find_empty_page(allocator).ok_or(Error::NoEntry)?;
        self.map(page.clone(), original_table_frame, allocator)?;
        let original_table = unsafe { &mut *(page.address() as *mut Table<Level1>) };

        // Override the recursive mapping.
        let entry = &mut self.level4_page_table_mut()[511];
        entry.set_frame_addr(inactive_page_table.level4_page_table.address());
        entry.set_flags(PageEntryFlags::WRITABLE);
        tlb::flush_all();

        let result = f(self, allocator);

        // Restore the active page table entry.
        let entry = &mut original_table[511];
        entry.set_frame_addr(addr);
        entry.set_flags(PageEntryFlags::WRITABLE);

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
    pub fn new(active_page_table: &mut ActivePageTable, allocator: &mut dyn FrameAllocator) -> Option<InActivePageTable> {
        allocator.alloc_one().map(|frame| {
            let p = active_page_table.find_empty_page(allocator).unwrap();
            active_page_table.map(p.clone(), frame.clone(), allocator).unwrap();
            let table = unsafe { &mut *(p.address() as *mut Table<Level1>) };
            table.clear_all_entries();

            // set recursive page mapping.
            let entry = &mut table[511];
            entry.set_frame_addr(frame.address());
            entry.set_flags(PageEntryFlags::WRITABLE);

            InActivePageTable { level4_page_table: frame }
        })
    }
}
