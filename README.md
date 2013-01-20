# Srjek's custom toolchain for the DCPU-16 #

Custom toolchain for the fictional [dcpu16](http://dcpu.com/dcpu-16/), created as part of Mojang's upcoming game [0x10c](http://0x10c.com/).

The assembler needs at least a partial rewrite, but the emulator is fairly stable and supports remote connections with a modified gdb.


####Download####
Clone this source repro, and everything besides the emulator is ready to go, provided python 3 is installed.

There is an included gdb port, run `git submodule update --init` to retrieve it (this may take a bit).
In the future, run `git submodule update` after a git pull to keep gdb up to date.


####Building####
GDB has it's own build instructions. Do `cd emulatorCpp/gdb` from the source directory, then follow the instructions found [here](https://github.com/srjek/gdb-dcpu16).

If you don't want to use the emulator, you can skip this.

First you will need to install any missing prerequisites:
 * [wxWidgets](http://www.wxwidgets.org/), for gui
 * [Boost](http://www.boost.org/), for networking (You will need to build boost, as the networking portion of boost depends on boost.System)
 * [Python 3](http://www.python.org/), needed to assemble the default boot rom
 * [freeglut](http://freeglut.sourceforge.net/), needed for opengl
 * [glew](http://glew.sourceforge.net/), needed for opengl
 * Optionally, makedepend, if you want to develop in the src directory (Makefiles in subdirs do not use makedepend)

Then, starting in the root of the source directory:

    cd emulatorCpp/src
    make

Done, run `./emulator --help` for usage.


####Some other smaller included tools####
 * `boot.dasm` is a floppy boot sector, and after being assembled, it can be used like `cat boot.bin yourprogram.bin > yourprogram.img` to create a bootable floppy image from any dcpu16 binary.
 * `convertEndian.cpp` is a tool to flip the endian-ness of a file, created when I accidently thought the endian-ness of a file was flipped.

####Oddities####
 * The assembler works, but doesn't have the most useful error messages at times, which is part of the reason for a partial rewrite
 * An older version of the emulator exists in the `emulator` folder, I can't justify getting rid of it because the automated self test hasn't been ported to and used on the newer emulator
 * If you really want to use the old emulator, you will need to install the included PyInline, and get it to work with your compiler.
   - Also, beware, the LEM1802 process may or may not start.
 * There is a `compiler` folder, while it may go somewhere in the future, it doesn't work, so use [llvm-dcpu16](https://github.com/llvm-dcpu16/llvm-dcpu16) or some other compiler
 * Speaking of experiments like the compiler, there is a `experimental` folder. Sooner or later I will probably split that into my own special branch, keeping just this repo for finished/usable code/tools.
 * There are some dcpu16 programs in the root folder, and others in the `testing` folder. Feel free to play around with them.
 
####TODO####
 * Clean up file organization, maybe make a root-level Makefile
 * Port test suite to the newer emulator, and get rid of the older emulator
 * Rewrite the assembler to be more helpful, even when it fails
 * When I can get [lldb](http://lldb.llvm.org/) to build under linux, use it to test that the emulator is speaking gdb-remote correctly
 
