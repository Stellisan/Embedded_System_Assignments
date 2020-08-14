#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>

//IOCTL command
#define ORD_VALUE _IOW('a','a',int*)

//structure for tracedata
struct tracedata{
	unsigned long Kaddress;
	pid_t process_id;
	uint32_t timestampcounter;
	int key;
	int data;
};

//Function for threads using the device rbt530_dev1
void *RBTree_Device1();

//Function for threads using the device rbt530_dev2
void *RBTree_Device2();

//Function for thread for Kprobe
void *Kprobe_test();

int main(int argc, char **argv)
{

	pthread_t thread1_device1, thread1_device2, thread2_device1, thread2_device2, thread_kprobe;
	
	// 2 threads for device 1
	pthread_create( &thread1_device1, NULL,RBTree_Device1,NULL);
	sleep(2);
	pthread_create( &thread2_device1, NULL,RBTree_Device1,NULL);

	// 2 threads for device 2
	pthread_create( &thread1_device2, NULL,RBTree_Device2,NULL);
	pthread_create( &thread2_device2, NULL,RBTree_Device2,NULL);

	// Thread for Kprobe
	pthread_create( &thread_kprobe, NULL, Kprobe_test, NULL);

	// wait for all the threads to complete	
	pthread_join(thread1_device1, NULL);
	pthread_join(thread2_device1, NULL);
	pthread_join(thread1_device2, NULL);
	pthread_join(thread2_device2, NULL);
	pthread_join(thread_kprobe, NULL);

	printf("Program Done....\n");
	return 0;
}

void *RBTree_Device1()
{
	printf("Inside Thread for device 1:\n");
	int filedescriptor, response;
	char buff[1024];
	int ord = 0;

	// Opening Device 1 rbt530_dev1
	filedescriptor = open("/dev/rbt530_dev1", O_RDWR);
	if(filedescriptor < 0)
	{
		printf("Could not open file...\n");
		return (void*)0;
	}
	else
	{
		printf("Device 1 file opened..\n");
	}

	// Adding 50 nodes into the tree
	for(int count =0;count < 50;count++)
	{
		memset(buff, 0, 1024);
		sprintf(buff,"%d",rand() % 50);
		response = write(filedescriptor, buff, strlen(buff)+1);
		if(response == strlen(buff)+1)
			printf("Can not write to the device file.\n");	
		else
			printf("Written into device 1: %s\n",buff);
		sleep(2);
		printf("Count.....%d\n",count);
	}
		
	// Read from the device file for 25 times
	for(int count =0;count < 25;count++)
	{		
		memset(buff, 0, 1024);
		response = read(filedescriptor, buff, 256);
		printf("Read from Device 1: '%s'\n", buff);
		sleep(2);
	}

	//changing the order to descending order
	ord = 1;
	ioctl(filedescriptor,ORD_VALUE,(int*)&ord);

	// Read from the device file for 25 times
	for(int count =0;count < 25;count++)
	{		
		memset(buff, 0, 1024);
		response = read(filedescriptor, buff, 256);
		printf("Read from Device 1: '%s'\n", buff);
		sleep(2);
	}

	close(filedescriptor);
	return (void*)1;
}

void *RBTree_Device2()
{
	int filedescriptor, response;
	char buff[1024];
	int ord = 0;

	printf("Inside Thread for device 2:\n");

	// Opening Device 2 rbt530_dev2
	filedescriptor = open("/dev/rbt530_dev2", O_RDWR);
	if(filedescriptor < 0)
	{
		printf("Could not open file...\n");
		return (void*)0;
	}
	else
	{
		printf("Device 2 file opened..\n");
	}


	// Adding 50 nodes into the tree
	for(int count =0;count < 50;count++)
	{
		memset(buff, 0, 1024);
		sprintf(buff,"%d", rand() % 50);
		printf("'%s'\n", buff);
		response = write(filedescriptor, buff, strlen(buff)+1);
		if(response == strlen(buff)+1)
			printf("Can not write to the device file.\n");	
		else
			printf("Written into device 2: %s\n",buff);
		printf("Count.....%d\n",count);	
		sleep(2);
	}
	
	// Read from the device file for 25 times
	for(int count =0;count < 25;count++)
	{
		memset(buff, 0, 1024);
		response = read(filedescriptor, buff, 256);
		sleep(1);
		printf("Read from Device 2: '%s'\n", buff);
	}
	
	//changing the order to descending order. By default it is ascending order
	ord = 1;
	ioctl(filedescriptor,ORD_VALUE,(int*)&ord);

	// Read from the device file for 25 times
	for(int count =0;count < 25;count++)
	{		
		memset(buff, 0, 1024);
		response = read(filedescriptor, buff, 256);
		printf("Read from Device 1: '%s'\n", buff);
		sleep(2);
	}

	close(filedescriptor);
	return (void*)1;
}

void *Kprobe_test()
{
	int filedescriptor, response;
	char function_name[256];

	printf("Inside Thread Kprobe: \n");

	// Open the device file rbprobe
	filedescriptor = open("/dev/rbprobe", O_RDWR);
	if(filedescriptor < 0)
	{
		printf("Could not open file...\n");
		return (void*)0;
	}

	memset(function_name, 0, 256);

	//User Input for function name
	printf("Enter the name of the function to add the probe: ");
	scanf("%s",function_name);

	//Pass the function name to the module
	response = write(filedescriptor, function_name, strlen(function_name)+1);
	if(response == strlen(function_name)+1)
		printf("Can not write to the device file.\n");	
	else
		printf("The function name,%s was sent to the probe\n",function_name);
	
	sleep(10);

	// Read the trace data from the kprobe module
	for(int index=0; index<3;index++)
	{	
		struct tracedata *read_buf = (struct tracedata *)malloc(sizeof(struct tracedata));
		if(read_buf == NULL)
			printf("Bad malloc\n");

		response =read(filedescriptor, read_buf , sizeof(struct tracedata));

		if (response==-1)
			printf("no data is read\n");
		else
		{	
			printf("\nreading from the ring buffer\n The TSC Value = %u\n The ADDR Value = %p\n The PID Value = %ld\n ", read_buf -> timestampcounter, (void*)read_buf->Kaddress, (long int)read_buf->process_id);
		}	
	}

	close(filedescriptor);
	return (void*)1;
}
