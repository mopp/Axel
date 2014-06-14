## Axel
Axel Accelerates All For Me !


## Requirement
clang, nasm, qemu(VBE), grub


## How to use ISO file
```shell:variable
qemu-system-i386 -monitor stdio -vga std -m 32 -cdrom axel.iso
```

## Setup Environment (NEW)
Please install requirements by using package manager or building from source.
If your clang is NOT enable cross-compiling, Please enable it.  
And If your host architecture is 64bit environment, You should install 32bit libc and etc.  
Because, Axel is 32bit yet :(


## Setup Environment (OLD)
Please read [GCC Cross-Compiler](http://wiki.osdev.org/GCC_Cross-Compiler "OSDev")  
If you cannot build below, you should NOT use source by repository.  
So, Please get and try to build each snapshot.  
  
  
environment variable  
```shell:variable
    # set install and target
    export PREFIX="$HOME/.mopp/axel"  
    export TARGET=i686-elf  
    export PATH="$PREFIX/bin:$PATH"  
```
  
binutils  
```shell:
    # Download Source
    git clone git://sourceware.org/git/binutils-gdb.git binutils
    cd binutils
    mkdir build
    cd build
    ../configure --target=$TARGET --prefix="$PREFIX"
    make
    make install
```
  
GNU GMP, GNU MPFR, and GNU MPC  
```shell:
    # Download Source for gcc
    wget https://ftp.gnu.org/gnu/gmp/gmp-5.1.3.tar.xz
    wget http://multiprecision.org/mpc/download/mpc-1.0.1.tar.gz
    wget http://www.mpfr.org/mpfr-current/mpfr-3.1.2.tar.xz
```
  
gcc  
```shell:
    # Download Source
    git clone git://gcc.gnu.org/git/gcc.git

    tar -xvf gmp-5.1.3.tar.xz
    mv gmp-5.1.3 ./gcc/gmp

    tar -xvf mpc-1.0.1.tar.gz
    mv mpc-1.0.1 ./gcc/mpc

    tar -xvf mpfr-3.1.2.tar.xz 
    mv mpfr-3.1.2 ./gcc/mpfr

    cd gcc
    mkdir build
    cd build
    ../configure --target=$TARGET --prefix="$PREFIX"  --enable-languages=c,c++ --without-headers 
    make all-gcc
    make all-target-libgcc
    make install-gcc
    make install-target-libgcc

    cd $PREFIX
    ln -s ../lib/gcc/i686-elf/4.9.0/include/* ./include 
```


## ScreenShot
![ss](./ss.png)
