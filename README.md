#How to run the kernel

1. make clean


2. ./build.sh

3. cd bin

  4.1 qemu-system-i386 -hda ./os.bin

  4.2 gdb

  4.2.1 add-symbol-file ../build/kernelfull.o 0x100000

  4.2.2 add-symbol-file ../XXXX 0xXXXXX (Used to state breakpoints from source files (aka break kernel.c:25))

  4.3 target remote | qemu-system-i386 -hda ./os.bin -S -gdb stdio