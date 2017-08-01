# Axel

[![license](https://img.shields.io/github/license/mashape/apistatus.svg)](LICENSE)

Axel Accelerates All For Me !  

Axel is general purpose operating system which is written by [Rust](https://www.rust-lang.org/) and some assembly languages([nasm](http://www.nasm.us/), ARM assembly).   
The current OS architecture is monolithic kernel.


## Support architecture
[] x86_32
[] x86_64
[] arm6 (Raspberry pi zero)


## Requirements
We need some tools to build Axel.
- gcc
- binutils
- rust (nightly)
- xargo
- nasm
- qemu
- ld
- make
- grub

In case of Arch Linux, you can install these software easily to execute below commands.
```console
# For ARM architecture.
yaourt -S arm-none-eabi-binutils arm-none-eabi-gcc arm-none-eabi-newlib

# For x86_32 architecture.
yaourt -S gcc-multilib nasm qemu
```


## Build
```console
make run_cdrom
```


## License
The MIT License (MIT)  
See [LICENSE](./LICENSE)


## :)
![LGTM](./axel_tan.png)
