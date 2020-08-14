/*
ASU ID: 1217643921
Name: Sanjay Arivazhagan
*/
#include <linux/netlink.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/netlink.h>
#include <linux/timer.h>
#include <linux/export.h>
#include <net/genetlink.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include "Genl_Netlink.h"

#define DRIVER_NAME  "spidev"
#define DEVICE_NAME  "myspidev"
#define MINOR_NUMBER    0
#define MAJOR_NUMBER   154 

#define GPIO(N) "GPIO"#N

static struct nla_policy genl_test_policy[GENL_TEST_ATTR_MAX+1] = {
	[GENL_TEST_ATTR_MSG] = {
		.type = NLA_UNSPEC,
	},
};

// Assembly code for time stamp counting
// Reference: https://stackoverflow.com/questions/9887839/how-to-count-clock-cycles-with-rdtsc-in-gcc-x86
#if defined(__i386__)

static __inline__ unsigned long long tsc_value(void)
{
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}

#elif defined(__x86_64__)

static __inline__ unsigned long long tsc_value(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#endif

// Netlink family
static struct genl_family genl_test_family;

// GPIOs for initializing SPI pins
static struct gpio request_gpio_arr[] = 
{
	{24, 0, "GPIO24"},
	{42, 0, "GPIO42"},
	{30, 0, "GPIO30"},
	{72, 0, "GPIO72"},
	{44, 0, "GPIO44"},
	{46, 0, "GPIO46"},
};

// Structure for spi device ids for platform devices
struct spi_device_id temp_spi[] = {
	{"spidev",0},
	{}
};

/* Structure for SPIDev */
struct spidev_data{
	dev_t devt;
	struct spi_device *spi;
	char pattern_buffer[4][8];
	unsigned int sequence_buffer[4][2];
};

struct class *led_dev_class;
static struct spidev_data *spidev;
static struct task_struct *ktask;	// Kthread task struct
static int irqno;
static unsigned int busyFlag = 0, busy_dist = 0;
static int ChipSelect, TriggerPin, EchoPin;	// Pins for SPI Led and Ultrasound sensors
static unsigned int speed = 600;		// Speed for the animation
struct spi_message msg;				// message to pass in SPI
uint64_t tsc1,tsc2;				// timestamps
uint64_t distance,time;				
static long distances = 0;
int buffer_length;
unsigned long flag;
unsigned char tx[2]={0};			
unsigned char rx[2]={0};
struct spi_transfer tr = {		// read/write buffer pair
			.tx_buf = &tx[0],	// data to be written
			.rx_buf = &rx[0],	// data to be read 
			.len = 2,		// size of the buffers
			.cs_change = 1,
			.bits_per_word = 8,
			.speed_hz = 500000, // speed of transfer for the device
			 };

/* Function declaration for kthread */
int kthread_display(void *data);

// Function name: 	led_disp_function
// Arguments: 		None
// Decription: 		Send the data to SPI using transfer function

static void transfer(unsigned char address, unsigned char data)
{
   	tx[0] = address;
    	tx[1] = data;
    
	/* Message initialised */
	spi_message_init(&msg);
	/* Message added to tail */
	spi_message_add_tail(&tr, &msg);
	/* Enable the Chip Select and send the message */
	gpio_set_value_cansleep(ChipSelect, 0);
	spi_sync(spidev->spi, &msg);
	gpio_set_value_cansleep(ChipSelect, 1);
}

// Function name: 	led_disp_function
// Arguments: 		None
// Decription: 		Send the data to SPI using transfer function
int led_disp_function(void)
{

	
	int i=0, j=0, k=0;
	
	/*If the initial sequence is 0,0 do not print anything */
	if(spidev->sequence_buffer[0][0] == 0 && spidev->sequence_buffer[0][1] == 0)
	{
		for(k=1; k < 9; k++)
		{
			transfer(k, 0x00);
		}
		busyFlag = 0;
		goto sequenceEnd;
	}
	
	/*If sequence pattern followed by 0,0 is present, then display the pattern in loop upto 0,0.*/
	for(j=0;j<4;j++) /* loop for sequence order */
	{
		for(i=0;i<4;i++)/* loop for pattern number */
		{	/*Found the required sequence from the pattern buffer */
			if(spidev->sequence_buffer[j][0] == i)
			{
				if((spidev->sequence_buffer[j][0] == 0) && (spidev->sequence_buffer[j][1] == 0))
				{/* 0,0 detected, clear the display*/
					busyFlag = 0;
					goto sequenceEnd;
				}
				else
				{
					/* Display the pattern for each of the 8 led rows*/
					transfer(0x01, spidev->pattern_buffer[i][0]);
					
					transfer(0x02, spidev->pattern_buffer[i][1]);
					
					transfer(0x03, spidev->pattern_buffer[i][2]);
					
					transfer(0x04, spidev->pattern_buffer[i][3]);
					
					transfer(0x05, spidev->pattern_buffer[i][4]);
					
					transfer(0x06, spidev->pattern_buffer[i][5]);
					
					transfer(0x07, spidev->pattern_buffer[i][6]);
					
					transfer(0x08, spidev->pattern_buffer[i][7]);
					
					msleep(spidev->sequence_buffer[j][1]);	
				}
			}
		}
	}
	sequenceEnd:
	busyFlag = 0;
	return 0;	
}


// Open for the led SPI device
static int led_open(struct inode *inode, struct file *file)
{
	int i=0;
	
	printk("\nOpen Device....\n");

	busyFlag = 0;
	
	// Initialize the Led Matrix
	transfer(0x0F, 0x01);
	transfer(0x0F, 0x00);
	transfer(0x09, 0x00);
	transfer(0x0A, 0x04);	
	transfer(0x0B, 0x07);
	transfer(0x0C, 0x01);

	// Clear the LED rows
	for(i=1; i < 9; i++)
	{
		transfer(i, 0x00);
	}

	return 0;
}


// release function of the device. Called when the device file is closed 
static int led_release(struct inode *inode, struct file *file)
{
	unsigned char i=0;
	int ret = 0;
	
	// Reinitialize speed and busyflag 
	busyFlag = 0;
	speed = 600;
   
    	// Clear Displays
	for(i=1; i < 9; i++)
	{
		transfer(i, 0x00);
	}
	
	// Free the GPIOS
	gpio_free(ChipSelect);
	gpio_free(24);
	gpio_free(30);
	gpio_free(42);
	gpio_free(72);
	gpio_free(44);
	gpio_free(46);
	gpio_free(EchoPin);
	gpio_free(TriggerPin);

	ret = kthread_stop(ktask);

	if(ret != 0)
		printk("Thread is closed");
	
	printk(KERN_DEBUG"spidev is closing\n");
	return 0;
}

// Command for setting the trigger, chip select and echo pins
static int genl_get_pinconfig(struct sk_buff* skb, struct genl_info* info)
{
	int ret;
	int Pins[3];
	if (!info->attrs[GENL_TEST_ATTR_MSG]) {
		printk(KERN_ERR "empty message from %d!!\n",
			info->snd_portid);
		printk(KERN_ERR "%p\n", info->attrs[GENL_TEST_ATTR_MSG]);
		return -EINVAL;
	}

	// Copying the data carried from the attribute to the array Pins
	ret = nla_memcpy((void*)Pins,info->attrs[GENL_TEST_ATTR_MSG],sizeof(Pins));

	if(ret == 0)
	{
		printk("Failure : Could not be copied.\n");
	}
		
	
	ChipSelect = Pins[0];
	TriggerPin = Pins[1];
	EchoPin = Pins[2];

	// Initialize the gpio pins
	gpio_free(TriggerPin);
	gpio_free(EchoPin);
	gpio_free(ChipSelect);

	gpio_request(ChipSelect,GPIO(ChipSelect));
	gpio_request(TriggerPin,GPIO(TriggerPin));
	gpio_request(EchoPin,GPIO(EchoPin));

	gpio_direction_output(ChipSelect,0);
	gpio_direction_input(EchoPin);
	gpio_direction_output(TriggerPin,0);

	gpio_set_value_cansleep(TriggerPin, 0);
	gpio_set_value_cansleep(ChipSelect, 1);
	
	return 0;
}

// Command Function for getting the patter from user
static int genl_user_pattern(struct sk_buff* skb, struct genl_info* info)
{
	int i=0, j=0;
	char writeBuffer[4][8];
	int ret;

	if (!info->attrs[GENL_TEST_ATTR_MSG]) {
		printk(KERN_ERR "empty message from %d!!\n",
			info->snd_portid);
		printk(KERN_ERR "%p\n", info->attrs[GENL_TEST_ATTR_MSG]);
		return -EINVAL;
	}

	ret = nla_memcpy((void*)writeBuffer,info->attrs[GENL_TEST_ATTR_MSG],sizeof(writeBuffer));

	if(ret == 0)
	{
		printk("Failure : Could not be copied.\n");
	}
	

	printk("\n%d\n",sizeof(writeBuffer));

	spidev->sequence_buffer[0][0] = 0;
	spidev->sequence_buffer[0][1] = speed;
	spidev->sequence_buffer[1][0] = 0;
	spidev->sequence_buffer[1][1] = 0;

	for(i=0;i<4;i++)
	{
		for(j=0;j<8;j++)
		{
			spidev->pattern_buffer[i][j] = writeBuffer[i][j];
		}
	}

	
	busyFlag = 1;
    	// Kthread_run creates and wakes up the task
   	 ktask =  kthread_run(&kthread_display, NULL,"kthread_spi_led");

	return ret;
}

// Interrupt function for Echo pin
static irqreturn_t hscr_interrupt_handler (int irq, void *dev_id)
{
	long time_difference;
	
	if (gpio_get_value(EchoPin))
	{
		// Timestamp for rising edge
		tsc1 = tsc_value();
		irq_set_irq_type(irq, IRQF_TRIGGER_FALLING);
	}
	else
	{
		// Timestamp for falling edge
		tsc2 = tsc_value();	
		time_difference = (long)(tsc2-tsc1);
		buffer_length++;
		if(buffer_length == 5)
		{
			// Checking the sample distance and changing the speed of the animation
			distances += distance;
			distances /= (long)5;
			printk("\nAverage_Distance: %ld \n",distances);
			if(distances < 100000)
				speed = 40;
			else if(100000 < distances && distances < 200000)
				speed = 100;
			else if(200000 < distances && distances < 300000)
				speed = 200;
			else
				speed = 400;
			buffer_length = 0;
			busy_dist = 0;
		}
		else
		{
			distances+=time_difference;
		}
		irq_set_irq_type(irq, IRQF_TRIGGER_RISING);
	}
	return IRQ_HANDLED;
}

// Function triggers the ultrasound sensors
static int genl_calculate_distance(struct sk_buff* skb, struct genl_info* info)
{
	int j = 0;
	if(busy_dist == 1)
		return 1;

	busy_dist = 1;

	// Triggering for 5 times
	for(j = 0; j<5; j++)
	{
		gpio_set_value_cansleep(TriggerPin,1);
		udelay(10);
		gpio_set_value_cansleep(TriggerPin,0);
		udelay(1300);
   	}

	return 0;
}

// Thread function for diaplaying the animation in LED Matrix
int kthread_display(void *data)
{
	int ret = 0;
	while(!kthread_should_stop())
	{
		spidev->sequence_buffer[0][0] = 0;
		spidev->sequence_buffer[0][1] = speed;
		spidev->sequence_buffer[1][0] = 0;
		spidev->sequence_buffer[1][1] = 0;
		ret = led_disp_function();
		msleep(100);

		
		spidev->sequence_buffer[0][0] = 1;
		spidev->sequence_buffer[0][1] = speed;
		spidev->sequence_buffer[1][0] = 0;
		spidev->sequence_buffer[1][1] = 0;
		
		ret = led_disp_function();
		msleep(100);
	}
	return 0;
}

// List of commands for Netlink
static const struct genl_ops genl_test_ops[] = {
	{
		.cmd = GENL_SEND_PIN_CONFIG,
		.doit = genl_get_pinconfig,
		.policy = genl_test_policy,
		.dumpit = NULL,
	},
	{
		.cmd = GENL_SEND_PATTERN,
		.doit = genl_user_pattern,
		.policy = genl_test_policy,
		.dumpit = NULL,
	},
	{
		.cmd = GENL_READ_DISTANCE,
		.doit = genl_calculate_distance,
		.policy = genl_test_policy,
		.dumpit = NULL,
	},
};

// Generic netlink family struct
static struct genl_family genl_test_family = {
	.name = GENL_TEST_FAMILY_NAME,	// name of family
	.version = 1,
	.maxattr = GENL_TEST_ATTR_MAX,
	.netnsok = false,
	.module = THIS_MODULE,
	.ops = genl_test_ops, // list of commands
	.n_ops = ARRAY_SIZE(genl_test_ops),
};

// Probe function for spidev
static int spidev_probe(struct spi_device *spi)
{
	struct device *dev;
	
	// Allocate driver data
	spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
	if (!spidev)
		return -ENOMEM;

	// Initialize the driver data
	spidev->spi = spi;

	spidev->devt = MKDEV(MAJOR_NUMBER, MINOR_NUMBER);

	// Create the device
	dev = device_create(led_dev_class, &spi->dev, spidev->devt, spidev, DEVICE_NAME);

	if(dev == NULL)
    	{
		printk("Device Creation Failed\n");
		kfree(spidev);
		return -1;
	}
	
	printk("LED Driver Probed \n");
	
	return 0;
}

// remove function for spidev
static int spidev_remove(struct spi_device *spi)
{
	device_destroy(led_dev_class, spidev->devt);
	kfree(spidev);
	printk("SPI LED Driver Removed.\n");
	return 0;
}

// The driver for spi
static struct spi_driver spi_led_driver = {
	.id_table = temp_spi,
        .driver = {
        	.name =         "spidev",
        	.owner =        THIS_MODULE,
        },
        .probe =        spidev_probe,
        .remove =       spidev_remove,
};

// Operations for the driver
static struct file_operations led_fops = {
  .owner   			= THIS_MODULE,
  .open    			= led_open,
  .release 			= led_release,
};


//Initialize function for the module
static int __init genl_test_init(void)
{
	int ret;
	
	printk(KERN_INFO "genl_test: initializing netlink\n");
	
	//Register the generic netlink family	
	ret = genl_register_family(&genl_test_family);

	buffer_length = 0;

	if (ret)
		return -EINVAL;
	else
		printk("\nGeneric netlink family name registered\n");

	// Register the Device
	ret = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &led_fops);
	if(ret < 0)
	{
		printk("\nDevice Registration Failed\n");
		return -1;
	}
	else
	{
		printk("\nDevice Registered\n");
	}
	
	// Create the class
	led_dev_class = class_create(THIS_MODULE, DEVICE_NAME);
	if(led_dev_class == NULL)
	{
		printk("\nClass Creation Failed\n");
		unregister_chrdev(MAJOR_NUMBER, spi_led_driver.driver.name);
		return -1;
	}
	else
	{
		printk("\nClass Created\n");
	}
	
	// Register the Driver
	ret = spi_register_driver(&spi_led_driver);
	if(ret < 0)
	{
		printk("\nDriver Registration Failed\n");
		class_destroy(led_dev_class);
		unregister_chrdev(MAJOR_NUMBER, spi_led_driver.driver.name);
		return -1;
	}
	else
	{
		printk("\nDriver Registered\n");
	}
	
	// Free gpio
	gpio_free(24);
	gpio_free(30);
	gpio_free(42);
	gpio_free(44);
	gpio_free(46);

	
	// Export the GPIO Pins
	gpio_request_array(request_gpio_arr,ARRAY_SIZE(request_gpio_arr));
	
	// Set the direction for the relevant GPIO pins
	gpio_direction_output(30,0);
	gpio_direction_output(42,0);
	gpio_direction_output(24,0);
	gpio_direction_output(44,0);
	gpio_direction_output(46,0);
	
	// Set the Value of GPIO Pins
	gpio_set_value_cansleep(30, 0);
	gpio_set_value_cansleep(42, 0);
	gpio_set_value_cansleep(24, 0);
	gpio_set_value_cansleep(72, 0);
	gpio_set_value_cansleep(44, 1);
	gpio_set_value_cansleep(46, 1);

	// Get the interrupt number for gpio
	irqno = gpio_to_irq(EchoPin);
	if(irqno < 0)
		printk(KERN_INFO "\nIRQ not registered\n");
	else
		printk(KERN_INFO "\nIRQ registered");
	
	// request for interrupt line
	ret = request_irq(irqno, hscr_interrupt_handler, IRQF_TRIGGER_RISING, GPIO(EchoPin),(void*)spidev);
	if (ret < 0)
	{
		printk(KERN_INFO "Failed to request IRQ line\n");
		if (ret != -EBUSY)
			ret = -EINVAL;
	}

	printk("\nLed Driver created\n");

	return 0;
}

module_init(genl_test_init);

// Exit function for the module
static void genl_test_exit(void)
{
	// unregister the netlink family
	genl_unregister_family(&genl_test_family);

	// free the irq line
	free_irq(irqno, spidev);
	
	spi_unregister_driver(&spi_led_driver);
	
	// Destroy device 
	device_destroy(led_dev_class, spidev->devt);

	// Delete cdev entries */
	unregister_chrdev(MAJOR_NUMBER, spi_led_driver.driver.name);

	// Destroy driver_class
	class_destroy(led_dev_class);
	
	printk("Spi Led Driver removed \n");
}
module_exit(genl_test_exit);

MODULE_LICENSE("GPL v2");