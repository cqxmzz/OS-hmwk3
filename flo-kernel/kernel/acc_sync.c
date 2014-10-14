/*
 * Read acceleration data and record it periodcally.
 *
 * (C) Copyright Team 1 @ 2014FALLW4118
 */
#include <linux/syscalls.h>
#include <linux/acceleration.h>
#include <linux/acc_sync.h>
#include <linux/acc_motion.h>
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
#include <linux/spinlock_types.h>
#include <asm/io.h>

/*Define the noise*/
#define NOISE 10

/*Define the window*/
#define WINDOW 20

#define MAX_PROCESS 4194303

/*
 * locking
 */
spinlock_t my_lock = __SPIN_LOCK_UNLOCKED();

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
	wait_queue_head_t *q;
	int sign;
	int head;
	int tail;
	int num_head;
	int num_tail;
};

struct motionArray array = {NULL, 0, 0};

/*
 * sensor data buffer
 */
int sensorDataBufferHead = 0;
struct dev_acceleration sensorDataBuffer[WINDOW+1];

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
	int tmp;
	void *pt;

	if (array.structs == NULL) {
		array.structs = kmalloc(10 * sizeof(struct motionStruct)
			, __GFP_NORETRY);
		if (array.structs == NULL)
			return -ENOMEM;
		array.size = 10;
		for (i = 0; i < 10; ++i) {
			array.structs[i].motion = NULL;
			array.structs[i].q = NULL;
			array.structs[i].sign = 0;
			array.structs[i].head = 0;
			array.structs[i].tail = 0;
			array.structs[i].num_head = 0;
			array.structs[i].num_tail = 0;
		}
		array.head = 0;
	}
	for (i = 0; i < array.size; ++i) {
		tmp = (i + array.head) % array.size;
		if (array.structs[tmp].motion == NULL
			&& array.structs[tmp].num_head == 0
			&& array.structs[tmp].num_tail == 0) {
			array.head = tmp;
			return tmp;
		}
	}
	if (array.size > 200)
		return -ENOMEM;
	pt = krealloc(array.structs
		, sizeof(struct motionStruct) * array.size * 2, __GFP_NORETRY);
	if (pt == NULL)
		return -ENOMEM;
	array.structs = pt;
	array.head = array.size;
	array.size = array.size * 2;
	for (i = array.size/2; i < array.size; ++i) {
		array.structs[i].motion = NULL;
		array.structs[i].q = NULL;
		array.structs[i].sign = 0;
		array.structs[i].head = 0;
		array.structs[i].tail = 0;
		array.structs[i].num_head = 0;
		array.structs[i].num_tail = 0;
	}
	return array.head;
}

SYSCALL_DEFINE1(accevt_create, struct acc_motion __user *, acceleration) {
	int place;
	struct acc_motion tmpMotion;

	if (acceleration == NULL)
		return -EINVAL;
	if (copy_from_user(&tmpMotion, acceleration, sizeof(struct acc_motion))
		!= 0)
		return -EINVAL;
	spin_lock(&my_lock);
	place = find_next_place();
	if (place < 0) {
		spin_unlock(&my_lock);
		return place;
	}
	array.structs[place].motion = kmalloc(sizeof(struct acc_motion)
		, __GFP_NORETRY);
	if (array.structs[place].motion == NULL)
		return -ENOMEM;
	array.structs[place].q = kmalloc(sizeof(wait_queue_head_t)
		, __GFP_NORETRY);
	if (array.structs[place].q == NULL) {
		kfree(array.structs[place].motion);
		array.structs[place].motion = NULL;
		return -ENOMEM;
	}
	array.structs[place].sign = 0;
	array.structs[place].head = 0;
	array.structs[place].tail = 0;
	array.structs[place].num_head = 0;
	array.structs[place].num_tail = 0;
	*(array.structs[place].motion) = tmpMotion;
	init_waitqueue_head(array.structs[place].q);
	if (array.structs[place].motion->frq > WINDOW)
		array.structs[place].motion->frq = WINDOW;
	spin_unlock(&my_lock);
	return place;
}

int smaller_than(int a, int b)
{
	if (a < b && a - b < MAX_PROCESS)
		return true;
	if (a > b && a - b > MAX_PROCESS)
		return true;
	return false;
}

/* Block a process on an event.
 * It takes the event_id as parameter. The event_id requires verification.
 * Return 0 on success and the appropriate error on failure.
 * system call number 380
 */
SYSCALL_DEFINE1(accevt_wait, int, event_id) {
	int ret;
	int num;
	DECLARE_WAITQUEUE(wait, current);

	spin_lock(&my_lock);
	if (array.structs[event_id].motion == NULL) {
		spin_unlock(&my_lock);
		return -EINVAL;
	}
	array.structs[event_id].num_head++;
	num = array.structs[event_id].head;
	array.structs[event_id].head++;
	array.structs[event_id].head %= (MAX_PROCESS * 2 + 10);
	add_wait_queue(array.structs[event_id].q, &wait);
	while (array.structs[event_id].motion != NULL
		&& !smaller_than(num, array.structs[event_id].sign)) {
		prepare_to_wait(array.structs[event_id].q, &wait
			, TASK_INTERRUPTIBLE);
		if (signal_pending(current)) {
			finish_wait(array.structs[event_id].q, &wait);
			array.structs[event_id].num_head--;
			spin_unlock(&my_lock);
			return -ERESTARTSYS;
		}
		spin_unlock(&my_lock);
		schedule();
		spin_lock(&my_lock);
	}
	finish_wait(array.structs[event_id].q, &wait);
	if (smaller_than(num, array.structs[event_id].sign)) {
		ret = 0;
		array.structs[event_id].num_tail--;
	} else {
		array.structs[event_id].num_head--;
		ret = -EINVAL;
	}
	if (array.structs[event_id].motion == NULL
		&& array.structs[event_id].num_tail == 0
		&& array.structs[event_id].num_head == 0) {
		kfree(array.structs[event_id].q);
		array.structs[event_id].q = NULL;
	} else if (array.structs[event_id].num_tail == 0)
		array.structs[event_id].tail = array.structs[event_id].sign;
	spin_unlock(&my_lock);
	return ret;
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
	int i;
	int j;
	int count;
	int difx;
	int dify;
	int difz;
	int sumx;
	int sumy;
	int sumz;
	struct dev_acceleration sensor_data;

	if (current->real_cred->uid)
		return -EACCES;
	if (acceleration == NULL)
		return -EINVAL;
	if (copy_from_user(&sensor_data, acceleration
		, sizeof(struct dev_acceleration)) != 0)
		return -EINVAL;
	spin_lock(&my_lock);
	add_buffer(&sensor_data);
	for (i = 0; i < array.size; ++i) {
		if (array.structs[i].motion == NULL)
			continue;
		count = 0;
		sumx = 0;
		sumy = 0;
		sumz = 0;
		for (j = 0; j < WINDOW; ++j) {
			if (j == sensorDataBufferHead)
				continue;
			difx = sensorDataBuffer[j].x - sensorDataBuffer[(j + 1)
				% WINDOW].x;
			dify = sensorDataBuffer[j].y - sensorDataBuffer[(j + 1)
				% WINDOW].y;
			difz = sensorDataBuffer[j].z - sensorDataBuffer[(j + 1)
				% WINDOW].z;
			difx = difx > 0 ? difx : -difx;
			dify = dify > 0 ? dify : -dify;
			difz = difz > 0 ? difz : -difz;
			if (difx + dify + difz <= NOISE)
				continue;
			count = count + 1;
			sumx += difx;
			sumy += dify;
			sumz += difz;
		}
		if (sumx > array.structs[i].motion->dlt_x
			&& sumy > array.structs[i].motion->dlt_y
			&& sumz > array.structs[i].motion->dlt_z
			&& count > array.structs[i].motion->frq) {
			array.structs[i].sign = array.structs[i].head;
			array.structs[i].num_tail += array.structs[i].num_head;
			array.structs[i].num_head = 0;
			wake_up(array.structs[i].q);
		}
	}
	spin_unlock(&my_lock);
	return 0;
}

/* Destroy an acceleration event using the event_id,
 * Return 0 on success and the appropriate error on failure.
 * system call number 382
 */
SYSCALL_DEFINE1(accevt_destroy, int, event_id) {
	spin_lock(&my_lock);
	kfree(array.structs[event_id].motion);
	array.structs[event_id].motion = NULL;
	wake_up(array.structs[event_id].q);
	spin_unlock(&my_lock);
	return 0;
}


