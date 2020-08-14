#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/init.h>
#include <linux/rbtree.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/kthread.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL v2");

//Device Names
#define DEVICE_ONE_NAME	"rbt530_dev1"
#define DEVICE_TWO_NAME	"rbt530_dev2"

//IOCTL command
// This command is only for reading values
#define ORD_VALUE _IOW('a','a',int*)

//Red Black Tree node structure
struct rb_object{
	struct rb_node node;
	int key;
	int data;
};

//Device Structure
struct rbt530_drv{
	struct cdev cdev;		//Character Device pointer
	char name[20];			//Name of device
	struct rb_root rbt;		//root for red black tree
	struct rb_node *rbt_currentnode;//node for reader
	int order;			//for traversal of tree 0 - ascending; 1-descending
}*rbt530_drvp1,*rbt530_drvp2;		//two pointers for the devices

static dev_t rbt530_dev_number;      	/* Allotted device number */

struct class *rbt530_dev_class;          	/* Tie with the device model */

//Devices
static struct device *rbt530_dev1;	
static struct device *rbt530_dev2;

//Mutex for the synchronization
static struct mutex lockmu;

//Driver Open
int rbt530_driver_open(struct inode *inode, struct file *file)
{
	struct rbt530_drv *rbt530_drvp;

	/* Get the per-device structure that contains this cdev */
	rbt530_drvp = container_of(inode->i_cdev, struct rbt530_drv, cdev);


	/* Easy access to cmos_devp from rest of the entry points */
	file->private_data = rbt530_drvp;
	printk("\n%s is opening \n", rbt530_drvp->name);
	return 0;
}

//Driver read function
ssize_t rbt530_driver_read(struct file *file, char *buf,
           size_t count, loff_t *ppos)
{
	
	int bytes_read = 0;
	char dat[256];

	//Get the driver structure from file
	struct rbt530_drv *rbt530_drvp = file->private_data;

	struct rb_node *node;
	struct rb_root *mytree = &(rbt530_drvp->rbt);	
	struct rb_object *data;
	
	printk("Reading from device....\n");
	node = mytree->rb_node;

	//Lock the mutex
	mutex_lock(&lockmu);

	// Check if the tree is empty
	if (!node)
	{
		printk("The tree is empty.");
		mutex_unlock(&lockmu);
		return -1;
	}

	//check if the current node is pointing to a node
	if(!(rbt530_drvp->rbt_currentnode))
	{
		printk("There is no node available.\n");
		mutex_unlock(&lockmu);
		return -1;
	}

	// Access node for the currentnode
	data = container_of(rbt530_drvp->rbt_currentnode, struct rb_object, node);

	sprintf(dat,"%d",data->data);
	printk("Reading-- %s\n",dat);

	// passing the data to the user program
	while (count && dat[bytes_read]) {
		
		put_user(dat[bytes_read], buf++);
		count--;
		bytes_read++;
	}

	//checking the order for moving to the next node
	if(rbt530_drvp->order == 0)
	{
		rbt530_drvp->rbt_currentnode = rb_next(rbt530_drvp->rbt_currentnode);
	} else {
		rbt530_drvp->rbt_currentnode = rb_prev(rbt530_drvp->rbt_currentnode);
	}

	mutex_unlock(&lockmu);
	return bytes_read;
}


//Driver write function
ssize_t rbt530_driver_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	int write_pointer = 0;
	int key = 0;
	int data = 0;
	static int p = 0;
	mutex_lock(&lockmu);
	struct rbt530_drv *rbt530_drvp = file->private_data;
	struct rb_root *mytree = &(rbt530_drvp->rbt);
	struct rb_node **new = &(mytree->rb_node);
	struct rb_node *parent = NULL;

	char data_to_write[256];
	memset(data_to_write, 0, 256);

	// get data from the user program
	while (count) {	
		get_user(data_to_write[write_pointer], buf++);
		count--;

		if(count){
			write_pointer++;
			if( write_pointer == 256)
				write_pointer = 0;
		}
	}

	kstrtol(data_to_write,10,(long *)&key);
	data = key%10;
	key = key/10;
	p++;
	printk("Writing -- Key -- %d \n", key);		
	printk("Writing -- data -- %d \n", data);
	
	// The tree is empty and the data is not equal to 0
	if(!(*new) && data != 0)
	{
		struct rb_object *da = container_of(*new, struct rb_object, node);
		da = kmalloc(sizeof(struct rb_object), GFP_KERNEL);
		da->key = key;
		da->data = data;
		rb_link_node(&da->node, NULL, new);
		rb_insert_color(&da->node,mytree);
		rbt530_drvp->rbt_currentnode = rb_first(mytree);
		mutex_unlock(&lockmu);
		return 0;
	}

	// iterate between nodes
	while(*new){
		struct rb_object *data = container_of(*new,struct rb_object,node);
		parent = *new;
		if(key < data->key)
		{
			new = &((*new)->rb_left);
		} else if(key > data->key){
			new = &((*new)->rb_right);
		} else {
			break;
		}
	}
	
	if(*new)
	{
		// If the node already exists
		struct rb_object *data_in_node = container_of(*new,struct rb_object,node);
		if(data != 0)
		{
			// if data is not zero, change the data
			data_in_node->data = data;
		}
		else
		{
			// Erase the node if the data is zero
			rb_erase((*new),mytree);
			kfree(container_of(*new, struct rb_object, node));
		}

	} else {
		// If the node does not exist create a new node
		struct rb_object *da = kmalloc(sizeof(struct rb_object), GFP_KERNEL);
		da->data = data;
		da->key = key;
		rb_link_node(&da->node, parent, new);
		rb_insert_color(&da->node,mytree);
	}

	mutex_unlock(&lockmu);
	return 0;
}

//Driver IOCTL function
static long rbt530_driver_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
	int value=0;
	struct rbt530_drv *rbt530_drvp = file->private_data;

	// Switch between commands and define what to be done for each command
	switch(ioctl_num){
		case ORD_VALUE:
			// Recieve value from user program for order
			copy_from_user(&value,(int*)ioctl_param,sizeof(value));
			printk("Value...%d\n",value);
			rbt530_drvp->order = value;

			if(value == 0)
				rbt530_drvp->rbt_currentnode = rb_first(&(rbt530_drvp1->rbt));
			else if(value == 1)
				rbt530_drvp->rbt_currentnode = rb_last(&(rbt530_drvp1->rbt));
		break;
	}
	
	return 0;
}

// Recursive function to release all the nodes in the ntree
void deletetree(struct rb_node *node, struct rb_root *tree)
{
	struct rb_object *data = container_of(node,struct rb_object,node);
	if(node == NULL)
		return;	

	deletetree(node->rb_left,tree);
	deletetree(node->rb_right,tree);
	
	rb_erase(node,tree);
	if(data != NULL)
	{
		kfree(data);
		data = NULL;
	}
	
}
//Driver release function
int rbt530_driver_release(struct inode *inode, struct file *file)
{
	printk("Device released.....");
	return 1;
}

// File operations structure. Defined in linux/fs.h
static struct file_operations rbt530_fops = {
    .owner		= THIS_MODULE,           /* Owner */
    .open		= rbt530_driver_open,        /* Open method */
    .release	= rbt530_driver_release,     /* Release method */
    .write		= rbt530_driver_write,       /* Write method */
    .read		= rbt530_driver_read,        /* Read method */
    .unlocked_ioctl              = rbt530_driver_ioctl,		/*IOCTL method*/
};

//Initialize Driver
int __init rbt530_driver_init(void)
{

	int ret;

	// Request dynamic allocation of a device major number 
	if (alloc_chrdev_region(&rbt530_dev_number, 0, 2, "RBTree") < 0) {
			printk(KERN_DEBUG "Can't register device One\n"); return -1;
	}

	// Populate sysfs entries
	rbt530_dev_class = class_create(THIS_MODULE, "RBTree");

	// Allocate memory for the per-device structures
	rbt530_drvp1 = kmalloc(sizeof(struct rbt530_drv), GFP_KERNEL);
	rbt530_drvp2 = kmalloc(sizeof(struct rbt530_drv), GFP_KERNEL);
		
	if (!rbt530_drvp1) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}
	if (!rbt530_drvp2) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}

	// Put the names of the devices into the device structures
	sprintf(rbt530_drvp1->name, DEVICE_ONE_NAME);
	sprintf(rbt530_drvp2->name, DEVICE_TWO_NAME);

	mutex_init(&lockmu);

	//Connect the file operations with the cdev and initialise the variables in the device structures
	cdev_init(&rbt530_drvp1->cdev, &rbt530_fops);
	rbt530_drvp1->cdev.owner = THIS_MODULE;

	cdev_init(&rbt530_drvp2->cdev, &rbt530_fops);
	rbt530_drvp2->cdev.owner = THIS_MODULE;

	rbt530_drvp1->rbt = RB_ROOT;
	rbt530_drvp2->rbt = RB_ROOT;

	rbt530_drvp1->order = 0;
	rbt530_drvp2->order = 0;

	// Connect the major/minor number to the cdev for device 1 and add to the system
	ret = cdev_add(&rbt530_drvp1->cdev, MKDEV(MAJOR(rbt530_dev_number),MINOR(rbt530_dev_number)), 1);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}
	
	// Connect the major/minor number to the cdev for device 2 and add to the system
	ret = cdev_add(&rbt530_drvp2->cdev, MKDEV(MAJOR(rbt530_dev_number),MINOR(rbt530_dev_number)+1),1);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}

	// Create the devices
	rbt530_dev1 = device_create(rbt530_dev_class, NULL, MKDEV(MAJOR(rbt530_dev_number), MINOR(rbt530_dev_number)), NULL, DEVICE_ONE_NAME);
	rbt530_dev2 = device_create(rbt530_dev_class, NULL, MKDEV(MAJOR(rbt530_dev_number), MINOR(rbt530_dev_number)+1), NULL, DEVICE_TWO_NAME);
	

	printk("RB tree driver initialized.");
	return 0;
}

//Exit Driver
void __exit rbt530_driver_exit(void)
{
	// Delete all the nodes of the tree in device 1
	struct rb_root *mytree = &(rbt530_drvp1->rbt);
	deletetree(rbt530_drvp1->rbt.rb_node,mytree);

	// Delete all the nodes of the tree in device 2
	mytree = &(rbt530_drvp2->rbt);
	deletetree(rbt530_drvp2->rbt.rb_node,mytree);

	// Release the major number
	unregister_chrdev_region((rbt530_dev_number), 2);

	// Destroy devices
	device_destroy (rbt530_dev_class, MKDEV(MAJOR(rbt530_dev_number), MINOR(rbt530_dev_number)));
	device_destroy (rbt530_dev_class, MKDEV(MAJOR(rbt530_dev_number), MINOR(rbt530_dev_number)+1));

	//Delete the character devices
	cdev_del(&rbt530_drvp1->cdev);
	kfree(rbt530_drvp1);
	cdev_del(&rbt530_drvp2->cdev);
	kfree(rbt530_drvp2);
	
	//Destroy driver_class
	class_destroy(rbt530_dev_class);
	
	printk("rbt530 driver removed.\n");
}

module_init(rbt530_driver_init);
module_exit(rbt530_driver_exit);

//Export the Symbols
EXPORT_SYMBOL(rbt530_driver_write);
EXPORT_SYMBOL(rbt530_driver_read);
