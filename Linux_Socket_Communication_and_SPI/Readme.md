Name: Sanjay Arivazhagan

The Kernel module in the program acts as a driver for LED matrix and ultrasound sensor and as a server for 
the netlink socket. The netlink is programmed with three commands

Command			Description
GENL_SEND_PIN_CONFIG	Pin configuration to kernel program
GENL_SEND_PATTERN	Send the pattern to kernel module
GENL_READ_DISTANCE	Command to read the distance using ultrasound sensor

The animation is done on a loop in the kernel module using kthreads.


Pin Configuration

Ultrasonic Sensor

Description	Pin
Vcc		5 V source
Echo		IO10
Trigger 	IO08
Ground		GND

Led Matrix device

Description	Pin
Vcc		5 V source
CS		IO12
MOSI/DIN	IO11
CLK		IO13

Steps to run the program:

Step 1: run the command 'sudo make' to compile and build the program.
Step 2: Copy the compiled code with the .ko object file and the exectable files to the galileo board
Step 3: insert the modules using the command 'insmod Kernel_Server.ko'
Step 4: run the user program. Command: ./genl_ex
