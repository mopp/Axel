## Axel
Axel Accelerates All For Me !

## Setup Environment
Please read [GCC Cross-Compiler](http://wiki.osdev.org/GCC_Cross-Compiler "OSDev")  
  
environment variable  
    # set install and target
    export PREFIX="$HOME/.mopp/axel"  
    export TARGET=i686-elf  
    export PATH="$PREFIX/bin:$PATH"  
  
binutils  
    # Download Source
    git clone git://sourceware.org/git/binutils-gdb.git binutils
    cd binutils
    mkdir build
    cd build
    ../configure --target=$TARGET --prefix="$PREFIX"
    make
    make install
  
GNU GMP, GNU MPFR, and GNU MPC  
    # Download Source for gcc
    wget https://ftp.gnu.org/gnu/gmp/gmp-5.1.3.tar.xz
    wget http://multiprecision.org/mpc/download/mpc-1.0.1.tar.gz
    wget http://www.mpfr.org/mpfr-current/mpfr-3.1.2.tar.xz
  
gcc  
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
    cd include
    ln -s ../lib/gcc/i686-elf/4.9.0/include/* ./include 
