#define SUITE_USER \
"INFO: - DLPI test suite should be run from normal user\n"
#define CANT_TEST \
"INFO: - This assertion can not be tested\n"
#define OPEN_FAIL \
"INFO: - DLPI open failure\n" 
#define CLOSE_FAIL \
"INFO: - DLPI close failure\n"
#define INFO_FAIL \
"INFO: - DLPI info request failed\n"
#define	BIND_FAIL \
"INFO: - DLPI bind failure\n" 
#define	UNBIND_FAIL \
"INFO: - DLPI unbind failure\n" 
#define IOCTL_FAIL \
"INFO: - DLPI ioctl failure\n"
#define SETUID_FAIL \
"INFO: - Attempt to setuid failed\n"
#define LINK_FAIL \
"INFO: - Link Failure\n"
#define SEND_FAIL \
"INFO: - Send data failure\n"
#define RCV_FAIL \
"INFO: - Receive data failure\n"
#define GETMACADDR_FAIL \
"INFO: - Couldn't get macaddr successfully, set in the configuration\n"
#define ALLOC_FAIL \
"INFO: - Memory Panic!!\n"
#define GETPANICADDR_FAIL \
"INFO: - Coudn't get panicaddr successfully\n"
#define NODE_NAME_FAIL \
"INFO: - All node names not found\n"
#define NET_INIT_FAIL \
"INFO: - Net initialization process failed\n"
#define CONNECTIVITY_FAIL \
"INFO: - The monitor node is not able to talk to it's neighbours\n"
#define NOT_CONFIGURED \
"INFO: - Localhost is not configured as a part of the Test Network\n"
#define RCVMSG_FAIL \
"INFO: - Receive data failed with 0x%x errno\n"
#define GETMSG_FAIL \
"INFO: - Getmsg failed with 0x%x errno\n"
#define PUTMSG_FAIL \
"INFO: - putmsg failed with 0x%x errno\n"
#define GETMSG_TIMEOUT_FAIL \
"INFO: - Warning: GETMSG_TIMEOUT parameter not specified\n"
#define AVOID_PANIC_FAIL \
"INFO: - Warning: AVOID_PANIC parameter not specified\n"
#define AVOID_PANIC \
"INFO: - Test not initiated to avoid possible panic\n"
#define GETMSG_TIMEOUT \
"INFO: - Getmsg timeout occurred\n"
#define CHILD_ABNORMAL_EXIT \
"INFO: - Child exited abnormally [%x]\n"
#define GEN_CONFIG_FAIL \
"INFO: - Network device name not given\n"
#define SAME_PHYSNET_FAIL \
"INFO: - Machines are not in same physical network\n"
#define NOT_ENOUGH_MACHINES \
"INFO: - Three machines needs to be configured\n"
#define WRONG_LEN_RECV \
"INFO: - Received a wrong length of data %x [expected %x]\n"
#define WRONG_SRC_ADDR \
"INFO: - DLPI fills in wrong source address in frame\n"
#define WRONG_SSAP \
"INFO: - DLPI fills in wrong source SAP address in frame\n"
#define NETDEV_FAIL \
"INFO:Network device name not specified\n"
#define NODECOUNT_FAIL \
"INFO:Number of machines in the test not specified\n"
