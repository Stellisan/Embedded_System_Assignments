#include <stdio.h> 
#include <fcntl.h>
#include <sys/types.h> 
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/wait.h>

#define INSDUMPS 359
#define RMDUMPS 360

// Function for pthread
void* create_directory(void* input){
	mkdir("H");
	return 0;
}


int main(int argc, char *argv[]) {
	pthread_t thread1;
	int stack_id,ret;
	int input = 0;
	char* symbolname = "sys_mkdir";
	pid_t yu;
	int remove_check = 0;
	int child_process = 0;

	printf("The process relationship in the programs.\nP1 - Parent process\nO - Owner Process\nPT2 - Thread from Owner Process \nC1 and C2 - Child Process of Owner\n");

	printf("        P1   \n"); 
	printf("      /    \\ \n"); 
	printf("    O     PT2 \n"); 
	printf("   /  \\  \n");
	printf("  C1    C2 \n");

	printf("\nEnter the mode depending on where to dump the stack(Input a Positive Integer):\n0 - Owner\n1 - Owner and its Siblings\n >1 - For Enable dump stack in all process\n::");
	scanf("%d",&input);

	printf("\nIf you want check if the program can remove the stack dump manually or automatically\n0 - Automatically\n1 - Manually\n::");
	scanf("%d",&remove_check);

	// Calling the syscall insdump
	stack_id = syscall(INSDUMPS,symbolname, input);

	child_process = fork();
	
	if(child_process == 0)
	{
		if(stack_id < 0){
			printf("\nCould not insert the Stack Dump. ERROR %d\n", errno);
			return -1;
		}
		sleep(2);
		mkdir("l");
		sleep(1);
	}
	else
	{	
		sleep(2);
		mkdir("P");
		int child_process = fork();
		if(child_process == 0)
		{
			// Calling the function from child process
			mkdir("O");
			
			// Calling the syscall rmdump
			ret = syscall(RMDUMPS,stack_id); 
			if(ret < 0){
				printf("Could not remove the Stack Dump. ERROR %d\n", errno);
				return -1;
			}
		}
		else
		{
			if(remove_check){
				// Calling the syscall rmdump
				ret = syscall(RMDUMPS,stack_id); 
				if(ret < 0){
					printf("Could not remove the Stack Dump. ERROR %d\n", errno);
					return -1;
				}
			}
			// Thread will have the same parent process as the main function
			pthread_create(&thread1,NULL, create_directory, (void *) NULL);
			pthread_join(thread1,NULL);
		}
	}


	return 0;
}