Kprobe Driver program: KprobeDriver.c
RB Tree Driver program: RBHDriver.c
User Program: main.c

The Kprobes can be added dynamicaly to the driver module, KBHDrive from the user program main.c. The KprobeDriver reads the symbol from the user and puts it into the kprobe to probe the kernel code. When the probe is hit, the pid, time stamp count and address are all passed into the tracedata structure. The main program has five threads. 4 thread for populating and doing operations on the trees and one thread to read a symbol from user and probe the kernel code.

Instruction:

Step 1. run command 'sudo make'
Step 2. Change the obj-m value in make file to 'RBHDriver.o'.
Step 3. run command 'sudo make'
Step 4. run command 'sudo insmod RBHDriver.ko'
Step 5. run command 'sudo insmod KprobeDriver.ko'
Step 6. run command 'sudo ./rbtree_tester'
Step 7. Type the function 'rbt530_driver_write' for the prompt from the program. 
Step 8. run command 'dmesg' to check the outputs kernel modules.
