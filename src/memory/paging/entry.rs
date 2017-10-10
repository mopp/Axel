bitflags! {
    pub struct PageEntryFlags: usize {
        const PRESENT         = 1 << 0;
        const WRITABLE        = 1 << 1;
        const USER_ACCESSIBLE = 1 << 2;
        const WRITE_THROUGH   = 1 << 3;
        const CACHE_DISABLE   = 1 << 4;
        const ACCESSED        = 1 << 5;
        const DIRTY           = 1 << 6;
        const HUGE_PAGE       = 1 << 7;
        const GLOBAL          = 1 << 8;
        const NO_EXECUTE      = 1 << 63;
    }
}


/// A page entry.
#[derive(Debug)]
pub struct PageEntry(usize);


impl PageEntry {
    /// Return `EntryFlags`.
    pub fn flags(&self) -> PageEntryFlags
    {
        PageEntryFlags::from_bits_truncate(self.0)
    }


    /// Clear the all flags (set the all flags zero).
    pub fn clear_all(&mut self)
    {
        self.0 = 0;
    }


    pub fn set_frame_addr(&mut self, addr: usize)
    {
        debug_assert_eq!(addr & 0xFFF, 0);
        debug_assert_eq!(addr & 0xFFFF_0000_0000_0000, 0);
        self.0 = self.0 | addr;
    }
}
