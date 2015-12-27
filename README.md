# Axel
Axel Accelerates All For Me !  
This is monolithic kernel written by [Rust](https://www.rust-lang.org/) and some assembly language([nasm](http://www.nasm.us/), ARM assembly).  
Support architectures are x86_32 and arm6 (Raspberry pi zero) now.


# Requirements
Need to build
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


# Building
```shell
cd src/
make
```

Then, execute it using qemu.
Note that it is only x86_32.
```shell
make run_kernel
```

Before executing above commands, we need to build required softwares (e.g., cross-compiler).  
I have used Arch Linux for developing.  
In case of Arch Linux, you can install these software easily to type below commands.
```shell
# For ARM architecture.
yaourt -S arm-none-eabi-binutils arm-none-eabi-gcc arm-none-eabi-newlib 

# For x86_32 architecture.
yaourt -S gcc-multilib nasm qemu
```


# License
The MIT License (MIT)  
See [LICENSE](./LICENSE.txt)


# :)
Axel-Girl.
![personification](./axel_tan.png)
