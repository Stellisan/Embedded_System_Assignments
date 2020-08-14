/*
ASU ID: 1217643921
Name: Sanjay Arivazhagan
*/
#ifndef GENL_TEST_H
#define GENL_TEST_H

#include <linux/netlink.h>

#ifndef __KERNEL__
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#endif

#define GENL_TEST_FAMILY_NAME		"genl_family_m"

#define GENL_TEST_ATTR_MSG_MAX		256
#define GENL_TEST_HELLO_INTERVAL	5000

enum {
	GENL_TEST_C_UNSPEC,		/* Must NOT use element 0 */
	GENL_SEND_PIN_CONFIG,
	GENL_SEND_PATTERN,
	GENL_READ_DISTANCE,
};

enum genl_test_attrs {
	GENL_TEST_ATTR_UNSPEC,		/* Must NOT use element 0 */

	GENL_TEST_ATTR_MSG,

	__GENL_TEST_ATTR__MAX,
};
#define GENL_TEST_ATTR_MAX (__GENL_TEST_ATTR__MAX - 1)



#endif
