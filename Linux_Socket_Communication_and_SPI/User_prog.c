/*
ASU ID: 1217643921
Name: Sanjay Arivazhagan
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include "Genl_Netlink.h"

#define SPI_DEVICE_NAME "/dev/myspidev"

// Function: prep_socket
// Arguments: netlink socket
// Description: Prepare the netlink socket.
static void prep_socket(struct nl_sock** nlsock)
{
	int family_id;
	
	*nlsock = nl_socket_alloc();
	if(!*nlsock) {
		fprintf(stderr, "Unable to alloc nl socket!\n");
		exit(EXIT_FAILURE);
	}

	nl_socket_disable_seq_check(*nlsock);
	nl_socket_disable_auto_ack(*nlsock);

	// connect to genl
	if (nl_connect(*nlsock,NETLINK_GENERIC)) {
		fprintf(stderr, "Unable to connect to genl!\n");
		goto exit_err;
	}

	// resolve the generic nl family id
	family_id = genl_ctrl_resolve(*nlsock, GENL_TEST_FAMILY_NAME);
	if(family_id < 0){
		fprintf(stderr, "Unable to resolve family name! \n");
		goto exit_err;
	}

    return;

exit_err:
    nl_socket_free(*nlsock);
    exit(EXIT_FAILURE);
}

// Function: send_msg_to_kernel
// Arguments: sock = socket, sel = command, datasize = size of the data, pattern = data
// Description: Prepare the netlink message with the data payload
static int send_msg_to_kernel(struct nl_sock *sock, int sel, int fam_id, int datasize, void* pattern)
{
	struct nl_msg* msg;
	int family_id, err = 0;

	family_id = fam_id;
	
	// allocate the netlink message
	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		exit(EXIT_FAILURE);
	}

	// Adds the generic header to the message
	if(!genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family_id, 0, 
		NLM_F_REQUEST, sel, 0)) {
		fprintf(stderr, "failed to put nl hdr!\n");
		err = -ENOMEM;
		goto out;
	}

	// Add the payload data
	err = nla_put(msg, GENL_TEST_ATTR_MSG,datasize,pattern);
	if (err) {
		fprintf(stderr, "failed to put nl string!\n");
		goto out;
	} else{
		printf("Message put.....\n");
	}	

	// Send the message to the sock
	err = nl_send_auto(sock, msg);
	if (err < 0) {
		fprintf(stderr, "failed to send nl message!\n");
	} else{
		printf("Message sent.....\n");
	}

// Incase of failure free the msg and return the error
out:
	nlmsg_free(msg);
	return err;
}

int main(int argc, char** argv)
{
	struct nl_sock* nlsock = NULL;
	int ret, fd, family_id;
	char choice;
	
	// Pattern dog from User space to kernel space
	char pattern[4][8] = {
		{0x08, 0x90, 0xf0, 0x10, 0x10, 0x37, 0xdf, 0x98},
		{0x20, 0x10, 0x70, 0xd0, 0x10, 0x97, 0xff, 0x18},
		{0x98, 0xdf, 0x37, 0x10, 0x10, 0xf0, 0x90, 0x08},
		{0x18, 0xff, 0x97, 0x10, 0xd0, 0x70, 0x10, 0x20},
		};
	
	int pin_configs[3] = {15,	// GPIO Pin for SPI Chip select
			   40,	// GPIO Pin for Trigger
			   10	// GPIO Pin for Echo
			};
	

	prep_socket(&nlsock);

	family_id = genl_ctrl_resolve(nlsock, GENL_TEST_FAMILY_NAME);

	if(family_id < 0){
		fprintf(stderr, "Unable to resolve family name!\n");
		exit(EXIT_FAILURE);
	}

	// Open device for SPI Led Matrix
	fd = open(SPI_DEVICE_NAME, O_RDWR);
	if(fd < 0)
	{
		printf("Can not open device file device file.\n");
		return 0;
	}

	printf("\nSending the pin configuration to kernel\n");
	ret = send_msg_to_kernel(nlsock,GENL_SEND_PIN_CONFIG,family_id,sizeof(pin_configs),(void*)pin_configs);

	if(ret < 0)
	{
		printf("\n Pin configuration not copied properly");
		return 1;
	}
	
	printf("\nSending the dog pattern to kernel\n");

	ret = send_msg_to_kernel(nlsock,GENL_SEND_PATTERN,family_id,sizeof(pattern),(void*)pattern);

	if(ret < 0)
	{
		printf("\n Pattern not copied properly");
		return 1;
	}	

	printf("\nDo you want to change the animation based the Ultrasound sensor (y/n): ");
	scanf("%c",&choice);

	if(choice == 'Y' || choice == 'y')
	{
		ret = send_msg_to_kernel(nlsock, GENL_READ_DISTANCE,family_id, sizeof(pattern),(void*)pattern);

		if(ret < 0)
		{
			printf("\n Message not send properly");
			return 1;
		}
	}
	sleep(10);
	
	close(fd);
	nl_socket_free(nlsock);
	return 0;
}