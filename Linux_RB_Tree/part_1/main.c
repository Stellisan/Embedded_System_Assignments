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

//Function for threads using the device rbt530_dev1
void *RBTree_Device1();
//Function for threads using the device rbt530_dev2
void *RBTree_Device2();

int main(int argc, char **argv)
{

	pthread_t thread1_device1, thread1_device2, thread2_device1, thread2_device2;
	
	// 2 threads for device 1
	pthread_create( &thread1_device1, NULL,RBTree_Device1,NULL);
	sleep(4);
	pthread_create( &thread2_device1, NULL,RBTree_Device1,NULL);

	// 2 threads for device 2
	pthread_create( &thread1_device2, NULL,RBTree_Device2,NULL);
	pthread_create( &thread2_device2, NULL,RBTree_Device2,NULL);

	// waiting for all the threads to complete
	pthread_join(thread1_device1, NULL);
	pthread_join(thread2_device1, NULL);
	pthread_join(thread1_device2, NULL);
	pthread_join(thread2_device2, NULL);

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
