/*
 *Define time interval (ms)
 */
 
#define TIME_INTERVAL  200

/*
 * Set current device acceleration in the kernel.
 * The parameter acceleration is the pointer to the address
 * where the sensor data is stored in user space.  Follow system call
 * convention to return 0 on success and the appropriate error value
 * on failure. 
 * syscall number 378
 */
 
int set_acceleration(struct dev_acceleration __user * acceleration) {
	return 0;
}

/*
 * The guide on how to get an instance of the default acceleration
 * sensor is available here.
 */

/*
 * The data structure to be used for passing accelerometer data to the
 * kernel and storing the data in the kernel.
 */

struct dev_acceleration {
	int x; /* acceleration along X-axis */
	int y; /* acceleration along Y-axis */
	int z; /* acceleration along Z-axis */
}; 