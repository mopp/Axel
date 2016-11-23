# Axel

[![license](https://img.shields.io/github/license/mashape/apistatus.svg)](LICENSE)

Axel Accelerates All For Me !  
This is monolithic kernel written by [Rust](https://www.rust-lang.org/) and some assembly languages([nasm](http://www.nasm.us/), ARM assembly).  
Support architectures are x86_32 and arm6 (Raspberry pi zero) currently.


# Requirements
We need some tools to build Axel.
- gcc (cross-compiler)
- binutils (cross)
- rust (nightly)
- cargo
- nasm
- qemu
- ld
- make

Optional
- grub

In case of Arch Linux, you can install these software easily to execute below commands.
```shell
# For ARM architecture.
yaourt -S arm-none-eabi-binutils arm-none-eabi-gcc arm-none-eabi-newlib 

# For x86_32 architecture.
yaourt -S gcc-multilib nasm qemu
```


# Building
```shell
make
make run_kernel
```


# License
The MIT License (MIT)  
See [LICENSE](./LICENSE)


# :)
![LGTM](./axel_tan.png)
