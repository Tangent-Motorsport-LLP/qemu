QEMU + gdb-tricore

SETUP
Qemu
- Download the current repo for qemu. Select develop branch.
- Use "./configure --target-list=tricore-softmmu --disable-werror" and "make" to build qemu
- Run command to launch qemu 
  "./qemu-system-tricore -M tricore_tsim161 -device loader,file=/home/rahul/bins/F4C9L8000.bin,addr=0x80000000 -singlestep -gdb tcp::4001 -S"

Gdb-tricore
- Download repo: https://github.com/volumit/gdb-tricore
- Configure gdb-tricore by using the following command: 
  configure --host=x86_64-linux-gnu --target=tricore-elf --disable-werror 
- Make build by command: make 
- Inside gdb-tricore repo launch gdb in gdb folder "gdb./gdb"
- Run "target remote localhost:4001" to connect to gdb server(should have same port number)

Some gdb commands:
- info frame: to get info on current frame
- ni: next instruction
- info registers: to get all registers and their values
- access memory: x 0x70000000
- set memory: set *0x70000000 = 0x20
- to run a specific code area=>
  set $pc=0x80007672
  break *0x80007862
  continue