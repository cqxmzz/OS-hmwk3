/*
 * Read acceleration data and record it periodcally.
 *
 * (C) Copyright Team 1 @ 2014FALLW4118
 */
#include <linux/syscalls.h>
#include <linux/acceleration.h>
#include <linux/acc_sync.h>
#include <linux/kernel.h>

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

/*Define the noise*/
#define NOISE 10

/*Define the window*/
#define WINDOW 20

/*
 * the list of motion
 */

struct motionArray {
	struct motionStruct *structs;
	int size;
	int head;
};

struct motionStruct {
     struct acc_motion *motion;
     //struct //queue
};

extern struct motionArray array;

struct motionArray array = {NULL, 0, 0};

/*
 * sensor data buffer
 */
int sensorDataBufferHead = 0;
struct dev_acceleration sensorDataBuffer[WINDOW];

extern struct dev_acceleration sensorDataBuffer[WINDOW];
extern int sensorDataBufferHead;

//lock?
void add_buffer(struct dev_acceleration *sensorData)
{
	sensorDataBufferHead = (sensorDataBufferHead + 1) % WINDOW;
	sensorDataBuffer[sensorDataBufferHead] = *sensorData;
}

/* Create an event based on motion.  
 * If frq exceeds WINDOW, cap frq at WINDOW.
 * Return an event_id on success and the appropriate error on failure.
 * system call number 379
 */

int find_next_place(void)
{
	int i;
	if (array.structs == NULL)
	{
		array.structs = kmalloc(10 * sizeof(struct motionStruct), __GFP_NORETRY);
		array.size = 10;
		array.head = 0;
	}
	for (i = 0; i < array.size; ++i)
	{
		int tmp = (i + array.head) % array.size;
		if (array.structs[tmp].motion == NULL) {
			array.head = tmp;
			return tmp;
		}
	}
	array.structs = krealloc(array.structs, sizeof(struct motionStruct) * array.size * 2, __GFP_NORETRY); //handle error
	array.head = array.size;
	array.size = array.size * 2;
	return array.head;
}

SYSCALL_DEFINE1(accevt_create, struct acc_motion __user *, acceleration) {
	int place;
	if (acceleration == NULL)
		return -EINVAL;
	place = find_next_place();//handle error
	array.structs[place].motion = kmalloc(sizeof(struct acc_motion), __GFP_NORETRY);
	if (copy_from_user(array.structs[place].motion, acceleration, sizeof(struct acc_motion)) != 0)
		return -EINVAL;
	if (array.structs[place].motion->frq > WINDOW)
		array.structs[place].motion->frq = WINDOW;
	//initialize wait queue
	return place;
}

/* Block a process on an event.
 * It takes the event_id as parameter. The event_id requires verification.
 * Return 0 on success and the appropriate error on failure.
 * system call number 380
 */
SYSCALL_DEFINE1(accevt_wait, int, event_id) {
	printk("add to wait queue");
	//add to wait queue
	return 0;
}

/* The acc_signal system call
 * takes sensor data from user, stores the data in the kernel,
 * generates a motion calculation, and notify all open events whose
 * baseline is surpassed.  All processes waiting on a given event 
 * are unblocked.
 * Return 0 success and the appropriate error on failure.
 * system call number 381
 */
SYSCALL_DEFINE1(accevt_signal, struct dev_acceleration __user *, acceleration) {
	struct dev_acceleration sensor_data;
	int i;
	int j;
	int count;
	int difx;
	int dify;
	int difz;
	int sumx;
	int sumy;
	int sumz;
	if (acceleration == NULL) {
		return -EINVAL;
	}
	if (copy_from_user(&sensor_data, acceleration, sizeof(struct dev_acceleration)) != 0) {
		return -EINVAL;
	}
	add_buffer(&sensor_data);
	printk("<0>""sensor data:%d,%d,%d\n", sensor_data.x, sensor_data.y, sensor_data.z);
	for (i = 0; i < array.size; ++i) {
		count = 0;
		sumx = 0;
		sumy = 0;
		sumz = 0;
		for (j = 0; j < WINDOW; ++j) {
			if (j == sensorDataBufferHead)
				continue;
			difx = sensorDataBuffer[j].x - sensorDataBuffer[(j + 1) % WINDOW].x;
			dify = sensorDataBuffer[j].y - sensorDataBuffer[(j + 1) % WINDOW].y;
			difz = sensorDataBuffer[j].z - sensorDataBuffer[(j + 1) % WINDOW].z;
			if (difx + dify + difz > NOISE)
				continue;
			count = count + 1;
			sumx += difx;
			sumy += dify;
			sumz += difz;
		}
		if (sumx > array.structs[i].motion->dlt_x 
			&& sumy > array.structs[i].motion->dlt_y 
			&& sumz > array.structs[i].motion->dlt_z 
			&& count > array.structs[i].motion->frq)
		printk("wake wait queue");
	}
	return 0;
}

/* Destroy an acceleration event using the event_id,
 * Return 0 on success and the appropriate error on failure.
 * system call number 382
 */
SYSCALL_DEFINE1(accevt_destroy, int, event_id) {
	printk("clear wait queue");
	kfree(array.structs[event_id].motion);//handle error
	array.structs[event_id].motion = NULL;
	return 0;
}


