**This is OLD VERSION**


# Axel
This is monolithic kernel written by C and nasm.  
And support architecture is only x86.


# Functions
- [x] Physical memory manager
- [x] x86_32 paging
- [x] Virtual memory manager
- [x] Interrupt
- [x] Mouse, Keyboard
- [x] Graphic(VBE)
- [x] ATA device control
- [ ] User process (WIP)
- [ ] System call (WIP)
- [ ] Virtual filesystem (WIP)
- [ ] Floppy boot (WIP)


# Requirements
We need these to build Axel.
- clang
- nasm
- qemu
- ld
- make
- grub

If your clang is NOT enable cross-compiling, Please enable it.  
And If your host architecture is 64bit environment, You should install 32bit libc and etc because this Axel is 32bit.

# Buid and Run
```shell
make run_qemu_cdrom
```

# ScreenShot
![ss](./ss.png)
