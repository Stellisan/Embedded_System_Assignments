Driver program: RBHDriver.c
User program: main.c

The RBHDriver program is kernel module driver program. Basic file operations for the device like read, write, open, IOCTL and release are implemented in it. On inserting the module, the module will create two character devices file, rb530_dev1 and rb530_dev2, and a tree for each device. It will also register the devices and driver. On removing the module, it will release the device numbers and destroy the devices.

When user program, main.c opens the device files and writes a data into the device file, the driver will add a node into the tree. The data passed from the user is a string of numbers, the last digit is the data and the rest is the key. The user program can assign the variable order in the module. According to which the tree traversal changes from ascending to descending according to the key. This is done using the functions rb_first, rb_next for ascending and rb_last, rb_prev for the descending order. The read function in the driver sends the data from the tree to the user program in ascending or descending order according to the variable order in the device structure. On closing the device file, the device is released. Adding the mutex lock, ensures that synchronization is possible.

Instructions to run the code.

1. type command 'sudo make' to build the .ko and out files.
2. Insert the module KBHDriver using the command 'sudo insmod RBHDriver.ko'
3. Command to run the user program, 'sudo ./rbtree_tester'
4. the messages from the kernel module can read using the command 'dmesg'
