Name: Sanjay Arivazhagan

Files Included
1. syspatch.patch
2. main.c
3. Makefile
4. Readme

Instructions for kernel level code

1. Extract the kernel source code into a folder, linux3.19-r0
2. copy the patch file 'syspatch.patch' into the above folder
3. patch the file to the source code using the command
	patch -p1 < syspatch.patch
4. cross compilation tools: export PATH=path_to_sdk/sysroots/x86_64-pokysdk-linux/usr/bin/i586-poky-linux:$PATH
5. Inside the folder kernel, run the command 'make menuconfig'
6. Select 'Kernel Hacking' -> 'Enable dynamic dump stack' press space and save the configuration.
7. compile the kernel: ARCH=x86 LOCALVERSION= CROSS_COMPILE=i586-poky-linux- make -j4
8. Load the bzimage file into sd card image for galileo board and boot the board

Instruction for User level program

1. Run 'sudo make'
2. Copy the executable files to the board
3. Inside the folder run the command './main'
 
Example Output:
./main
The process relationship in the programs.
P1 - Parent process
O - Owner Process
PT2 - Sybling of Owner Process
C1 and C2 - Child Process of Owner
        P1
      /    \
    O     PT2
   /  \
  C1    C2

Enter the mode depending on where to dump the stack(Input a Positive Integer):
0 - Owner
1 - Owner and its Siblings
 >1 - For Enable dump stack in all process
::1

If you want check if the program can remove the stack dump manually or automatically
0 - Automatically
1 - Manually
::0
[  255.734673] Inside Syscall insdump:

[  255.754219] kprobe inserted at c112a760 from Owner PID: 318
Owner and Sibling
[  257.764208] CPU: 0 PID: 318 Comm: fr Not tainted 3.19.8-yocto-standard #34
[  257.771124] Hardware name: Intel Corp. QUARK/GalileoGen2, BIOS 0x01000200 01/                                   01/2014
[  257.772177]  cd77b184 cd77b184 ce60bf3c c1453a01 ce60bf48 c12648af c15b02e3 c                                   e60bf60
[  257.772177]  c10a2772 cd77b18c 00000246 00000001 080485a0 ce60bf6c c102853a 0                                   8048bd2
[  257.772177]  ce60a000 d277705c 08048bd2 4c09caf6 000000e0 00000001 080485a0 c                                   e60a000
[  257.772177] Call Trace:
[  257.772177]  [<c1453a01>] dump_stack+0x16/0x18
[  257.772177]  [<c12648af>] Pre_Handler+0x5f/0x90
[  257.772177]  [<c10a2772>] opt_pre_handler+0x32/0x60
[  257.772177]  [<c102853a>] optimized_callback+0x5a/0x70
[  257.772177]  [<c1070000>] ? unmask_threaded_irq+0x30/0x40
[  257.772177]  [<c112a761>] ? SyS_mkdir+0x1/0xd0
[  257.772177]  [<c1457404>] ? syscall_call+0x7/0x7
[  257.852908]
[  257.852908] Inside Syscall rmdump:
[  257.857849]
[  257.857849] Could not remove the Stack Dump from PID: 320
[  257.864953]
[  257.864953] Owner and Sibling
[  257.869471] CPU: 0 PID: 321 Comm: fr Not tainted 3.19.8-yocto-standard #34
[  257.874809] Hardware name: Intel Corp. QUARK/GalileoGen2, BIOS 0x01000200 01/                                   01/2014
[  257.874809]  cd77b184 cd77b184 ce625f3c c1453a01 ce625f48 c12648af c15b02e3 c                                   e625f60
[  257.874809]  c10a2772 cd77b18c 00000246 00000000 b77c4b40 ce625f6c c102853a 0                                   80489a4
[  257.874809]  ce624000 d277705c 080489a4 4bf00740 000000e0 00000000 b77c4b40 c                                   e624000
[  257.874809] Call Trace:
[  257.874809]  [<c1453a01>] dump_stack+0x16/0x18
[  257.874809]  [<c12648af>] Pre_Handler+0x5f/0x90
[  257.874809]  [<c10a2772>] opt_pre_handler+0x32/0x60
[  257.874809]  [<c102853a>] optimized_callback+0x5a/0x70
[  257.874809]  [<c112a761>] ? SyS_mkdir+0x1/0xd0
[  257.874809]  [<c1457404>] ? syscall_call+0x7/0x7
Could not remove the Stack Dump. ERROR 22
[  257.958558]
[  257.958558] Inside Removal Function...
[  257.963942] Process 318 is exiting and the stack dump is cleared