use core::fmt;
use crate::memory::address::PhysicalAddress;

bitflags! {
    pub struct PageEntryFlags: usize {
        const Present        = 1 << 0;
        const Writable       = 1 << 1;
        const UserAccessible = 1 << 2;
        const WriteThrough   = 1 << 3;
        const CacheDisable   = 1 << 4;
        const Accessed       = 1 << 5;
        const Dirty          = 1 << 6;
        const HugePage       = 1 << 7;
        const Global         = 1 << 8;
        const NoExecute      = 1 << 63;
    }
}

/// A page entry.
#[derive(Debug)]
pub struct PageEntry(usize);

impl PageEntry {
    /// Return `EntryFlags`.
    pub fn flags(&self) -> PageEntryFlags {
        PageEntryFlags::from_bits_truncate(self.0)
    }

    pub fn set_flags(&mut self, flags: PageEntryFlags) {
        self.0 = self.0 | (self.flags() | flags).bits();
    }

    /// Clear the all flags (set the all flags zero).
    pub fn clear_all(&mut self) {
        self.0 = 0;
    }

    pub fn set_frame_addr(&mut self, addr: PhysicalAddress) {
        debug_assert_eq!(addr & 0xFFF, 0);
        debug_assert_eq!(addr & 0xFFFF_0000_0000_0000, 0);
        self.0 = self.0 | addr;
        self.set_flags(PageEntryFlags::Present);
    }

    pub fn get_frame_addr(&self) -> Option<PhysicalAddress> {
        if self.flags().contains(PageEntryFlags::Present) {
            Some(self.0 & !0xFFF)
        } else {
            None
        }
    }
}

impl fmt::Display for PageEntry {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.0)
    }
}

impl fmt::LowerHex for PageEntry {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:016x}", self.0)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_flags() {
        let e = PageEntry(0xFFFF_FFFF_FFFF_FF00);
        assert_eq!(false, e.flags().contains(PageEntryFlags::Present));
    }
}
