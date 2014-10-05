/*
 * Read acceleration data and record it periodcally.
 *
 * (C) Copyright Team 1 @ 2014FALLW4118
 */


#include <linux/cred.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/threads.h>
#include <linux/syscalls.h>
#include <linux/init_task.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <asm/io.h>

/*
 *Define time interval (ms)
 */
 
#define TIME_INTERVAL  200
/*
 * The data structure to be used for passing accelerometer data to the
 * kernel and storing the data in the kernel.
 */

struct dev_acceleration {
	int x; /* acceleration along X-axis */
	int y; /* acceleration along Y-axis */
	int z; /* acceleration along Z-axis */
};
/*
 * Set current device acceleration in the kernel.
 * The parameter acceleration is the pointer to the address
 * where the sensor data is stored in user space.  Follow system call
 * convention to return 0 on success and the appropriate error value
 * on failure. 
 * syscall number 378
 struct dev_acceleration __user * acceleration
 */
 
SYSCALL_DEFINE1(set_acceleration, int, test) {
	return test;
}

/*
 * The guide on how to get an instance of the default acceleration
 * sensor is available here.
 */

 