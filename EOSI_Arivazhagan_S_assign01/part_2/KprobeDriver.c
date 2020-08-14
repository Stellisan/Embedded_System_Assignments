#include <linux/init.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>

#define DEVICE_NAME                 "rbprobe"  // device name to be created and registered
#define BUFFER_SIZE			3	// Fixed buffer size for the trace data buffer
static struct kprobe kp;		// Kprobe structre

// Device Structure
struct rbprobe_drv {
	struct cdev cdev;               // The cdev structure
	char name[20];                  // Name of device
	char function_name[256];			// buffer for the input string */
	int current_write_pointer;
	int registered;			
} *rbprobe_devp;		//Pointer to the device structure 

// Structure for the trace data
struct tracedata{
	unsigned long Kaddress;	// Address taken by the probe
	pid_t process_id;	// process id in which the function runs
	uint32_t timestampcounter;	// Time stamp counter for the function
	int key;		// key of the tree node
	int data;		// data of the tree node
} **tracebuffer;

static int bufferindex = 0;	// Index for adding the trace datas
static int tracedatacount = 0;	// Count for the number of trace datas
static int bufferread = 0;	// Index for reading the trace data buffer

static dev_t rbprobe_dev_number;      // Allotted device number
struct class *rbprobe_dev_class;          // Tie with the device model
static struct device *rbprobe_dev_device;

// Assembly code for time stamp counting
// Reference: https://stackoverflow.com/questions/9887839/how-to-count-clock-cycles-with-rdtsc-in-gcc-x86
#if defined(__i386__)

static __inline__ unsigned long long rdtsc(void)
{
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}

#elif defined(__x86_64__)

static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#endif

// kprobe pre_handler: called just before the probed instruction is executed
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	uint32_t tsc;
	pid_t pid;

	printk(KERN_INFO "<%s> pre_handler: p->addr = 0x%p, ip = %lx,"
			" flags = 0x%lx\n",
		p->symbol_name, p->addr, regs->ip, regs->flags);

	// For time satmp
	tsc =rdtsc();
	printk("TSC = %u \n", tsc);
	// For process id
	pid = task_pid_nr(current);
        printk("PID = %d\n", pid);
				
	//Passing the trace data into the structure
	if(tracebuffer)
	{
		tracebuffer[bufferindex]->Kaddress = (unsigned long)p->addr;
		tracebuffer[bufferindex]->process_id = pid;
		tracebuffer[bufferindex]->timestampcounter = tsc;
		bufferindex = (bufferindex +1) % BUFFER_SIZE;
		tracedatacount = tracedatacount % BUFFER_SIZE + 1;
 	 }
	return 0;
}

/* kprobe post_handler: called after the probed instruction is executed */
static void handler_post(struct kprobe *p, struct pt_regs *regs,
				unsigned long flags)
{

}

/*
 * fault_handler: this is called if an exception is generated for any
 * instruction within the pre- or post-handler, or when Kprobes
 * single-steps the probed instruction.
 */
static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	printk(KERN_INFO "fault_handler: p->addr = 0x%p, trap #%dn",
		p->addr, trapnr);
	return 0;
}

// Driver Open
int rbprobe_driver_open(struct inode *inode, struct file *file)
{
	struct rbprobe_drv *rbprobe_devp;

	// Get the per-device structure that contains this cdev
	rbprobe_devp = container_of(inode->i_cdev, struct rbprobe_drv, cdev);

	// Easy access to cmos_devp from rest of the entry points
	file->private_data = rbprobe_devp;

	// Allocating memory for trace data structure
	tracebuffer = kmalloc(sizeof(struct tracedata*), GFP_KERNEL);
	*tracebuffer = kmalloc(sizeof(struct tracedata)*BUFFER_SIZE, GFP_KERNEL);

	printk("\nrbprobe is opened \n");

	return 0;
}

// Driver Read
ssize_t rbprobe_driver_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	int bytes_read;	
	
	// Send the data to user program
	if(tracedatacount >= 1)
	{
		if (copy_to_user((struct tracedata*)buf, tracebuffer[bufferread] , sizeof(struct tracedata)))
        	{	
			printk("unable to copy to  the user");
			return -1;
		}
		// Index for reading the buffer
		bytes_read = sizeof(struct tracedata);
		bufferread = (bufferread + 1)% BUFFER_SIZE;
		return bytes_read;
	}
	else
	{
		printk("Buffer not yet populated\n");
		return -1;
	}
	return 0;
	
}

// Driver Write
ssize_t rbprobe_driver_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	struct rbprobe_drv *rbprobe_devp = file->private_data;
	int res = 0;
	
	// Getting the Symbol from user program
	while (count) {	
		get_user(rbprobe_devp->function_name[rbprobe_devp->current_write_pointer], buf++);
		count--;
		if(count){
			rbprobe_devp->current_write_pointer++;
			if( rbprobe_devp->current_write_pointer == 256)
				rbprobe_devp->current_write_pointer = 0;
		}
	}
	printk("Writing -- %s \n", rbprobe_devp->function_name);

	kp.symbol_name = rbprobe_devp->function_name;

	// Registering and unregistering the the Kprobe 
	if(rbprobe_devp->registered ==0)
	{
		res= register_kprobe(&kp);
		if (res < 0) {
		printk(KERN_INFO "Kprobe not registered %d\n", res);
		return res;}
		else
		{printk ("Kprobe is registered");
		rbprobe_devp->registered =1;
		}
	}
	else
	{	unregister_kprobe(&kp);
		rbprobe_devp->registered = 0;
	}		

	return 0;
}

// Driver Release
int rbprobe_driver_release(struct inode *inode, struct file *file)
{
	kfree(*tracebuffer);
	unregister_kprobe(&kp);

	printk("Device is released\n");
	
	return 0;
}

// File Operations
static struct file_operations rbprobe_fops = {
	.owner          = THIS_MODULE,
	.read           = rbprobe_driver_read,
	.write          = rbprobe_driver_write,
	.open           = rbprobe_driver_open,
	.release        = rbprobe_driver_release,
};

// Initialze Driver
static int __init rbprobe_init(void)
{
	int ret;
    
	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&rbprobe_dev_number, 0, 1, DEVICE_NAME) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	/* Populate sysfs entries */
	rbprobe_dev_class = class_create(THIS_MODULE, DEVICE_NAME);
	
	/* Allocate memory for the per-device structure */
	rbprobe_devp = kmalloc(sizeof(struct rbprobe_drv), GFP_KERNEL);
		
	if (!rbprobe_devp) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}

	memset(rbprobe_devp->function_name, 0, 256);	

	// Request I/O region
	sprintf(rbprobe_devp->name, DEVICE_NAME);

	// Connect the file operations with the cdev
	cdev_init(&rbprobe_devp->cdev, &rbprobe_fops);
	rbprobe_devp->cdev.owner = THIS_MODULE;

	// Connect the major/minor number to the cdev
	ret = cdev_add(&rbprobe_devp->cdev, (rbprobe_dev_number), 1);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}

	// Create Devices
	rbprobe_dev_device = device_create(rbprobe_dev_class, NULL, MKDEV(MAJOR(rbprobe_dev_number), 0), NULL, DEVICE_NAME);

	kp.pre_handler = handler_pre;
	kp.post_handler = handler_post;
	kp.fault_handler = handler_fault;
	
	printk("rbprobe driver registered.....\n");

	return 0;
}

//Exit Driver
static void __exit rbprobe_exit(void)
{
	// Release the major number
	unregister_chrdev_region((rbprobe_dev_number), 1);

	// Destroy device
	device_destroy (rbprobe_dev_class, MKDEV(MAJOR(rbprobe_dev_number), 0));
	cdev_del(&rbprobe_devp->cdev);
	kfree(rbprobe_devp);
	
	// Destroy driver_class
	class_destroy(rbprobe_dev_class);
	printk("rbprobe driver removed.\n");
}

module_init(rbprobe_init)
module_exit(rbprobe_exit)
MODULE_LICENSE("GPL v2");
