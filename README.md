#How to run the kernel

* make clean


* ./build.sh

* cd bin

    * qemu-system-i386 -hda ./os.bin

    * gdb

        * add-symbol-file ../build/kernelfull.o 0x100000

        * add-symbol-file ../XXXX 0xXXXXX (Used to state breakpoints from source files (aka break kernel.c:25))

        * target remote | qemu-system-i386 -hda ./os.bin -S -gdb stdio