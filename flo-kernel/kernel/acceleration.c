/*
 * Read acceleration data and record it periodcally.
 *
 * (C) Copyright Team 1 @ 2014FALLW4118
 */
#include <linux/syscalls.h>
#include <linux/dev_acceleration.h>
#include <linux/kernel.h>

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
*/
extern struct dev_acceleration sensorData;

/*
 * Set current device acceleration in the kernel.
 * The parameter acceleration is the pointer to the address
 * where the sensor data is stored in user space.  Follow system call
 * convention to return 0 on success and the appropriate error value
 * on failure. 
 * syscall number 378
 */
 
SYSCALL_DEFINE1(acceleration, struct dev_acceleration __user *, buf) {
	struct dev_acceleration sensorData;
	if (buf == NULL) {
		return -EINVAL;
	} 
	if (copy_from_user(&sensorData, buf, sizeof(struct dev_acceleration)) != 0) {
		return -EINVAL;
	}
	printk("<0>""sensorData:%d\n", sensorData.x);
	return 0;
}




 