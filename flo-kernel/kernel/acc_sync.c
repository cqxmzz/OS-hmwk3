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
#include <linux/spinlock_types.h>
#include <asm/io.h>

/*Define the noise*/
#define NOISE 10

/*Define the window*/
#define WINDOW 20

/*
 * locking
 */
extern spinlock_t my_lock;
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
     wait_queue_head_t *waking;
     int flag;
     int num;
     int waking_num;
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
	if (array.structs == NULL)
	{
		array.structs = kmalloc(10 * sizeof(struct motionStruct), __GFP_NORETRY);
		array.size = 10;
		for (i = 0; i < 10; ++i) {
			array.structs[i] = {NULL, NULL, NULL, 0, 0, 0};
		}
		array.head = 0;
	}
	for (i = 0; i < array.size; ++i)
	{
		tmp = (i + array.head) % array.size;
		if (array.structs[tmp].motion == NULL && array.structs[tmp].num == 0 && array.structs[tmp].waking_num == 0) {
			array.head = tmp;
			return tmp;
		}
	}
	if (array.size > 100)
		return -ENOMEM;
	pt = krealloc(array.structs, sizeof(struct motionStruct) * array.size * 2, __GFP_NORETRY);
	if (pt == NULL)
		return -ENOMEM;
	array.structs = pt;
	array.head = array.size;
	array.size = array.size * 2;
	for (i = array.size/2; i < array.size; ++i) {
		array.structs[i] = {NULL, NULL, NULL, 0, 0, 0};
	}
	return array.head;
}

SYSCALL_DEFINE1(accevt_create, struct acc_motion __user *, acceleration) {
	int place;
	struct acc_motion tmpMotion;
	if (acceleration == NULL)
		return -EINVAL;
	if (copy_from_user(&tmpMotion, acceleration, sizeof(struct acc_motion)) != 0)
		return -EINVAL;
	spin_lock(&my_lock);
	place = find_next_place();
	if (place < 0) {
		spin_unlock(&my_lock);
		return place;
	}
	array.structs[place].motion = kmalloc(sizeof(struct acc_motion), __GFP_NORETRY);
	array.structs[place].q = kmalloc(sizeof(wait_queue_head_t), __GFP_NORETRY);
	array.structs[place].waking = kmalloc(sizeof(wait_queue_head_t), __GFP_NORETRY);
	array.structs[place].flag = 0;
	array.structs[place].num = 0;
	array.structs[place].waking_num = 0;
	*(array.structs[place].motion) = tmpMotion;
	init_waitqueue_head(array.structs[place].q);
	init_waitqueue_head(array.structs[place].waking);
	if (array.structs[place].motion->frq > WINDOW)
		array.structs[place].motion->frq = WINDOW;
	spin_unlock(&my_lock);
	return place;
}

/* Block a process on an event.
 * It takes the event_id as parameter. The event_id requires verification.
 * Return 0 on success and the appropriate error on failure.
 * system call number 380
 */
SYSCALL_DEFINE1(accevt_wait, int, event_id) {
	int ret;
	DECLARE_WAITQUEUE(wait1,current);
	DECLARE_WAITQUEUE(wait2,current);
	
	spin_lock(&my_lock);
	if (array.structs[event_id].flag == 2) {
		spin_unlock(&my_lock);
		return -EINVAL;
	}
	array.structs[event_id].waking_num++;
	if (array.structs[event_id].flag == 1) {
		spin_unlock(&my_lock);
	}
	add_wait_queue(array.structs[event_id].waking, &wait1);
	while (array.structs[event_id].flag == 1) {
		prepare_to_wait(array.structs[event_id].waking, &wait1, TASK_INTERRUPTIBLE);
		schedule();
	}
	finish_wait(array.structs[event_id].waking, &wait1);
	spin_lock(&my_lock);
	array.structs[event_id].waking_num--;
	if (array.structs[event_id].flag == 2) {
		if (array.structs[event_id].waking_num == 0) {
			kfree(array.structs[event_id].waking);
			array.structs[event_id].waking = NULL;
		}
		spin_unlock(&my_lock);
		return -EINVAL;
	}
	array.structs[event_id].num++;
	spin_unlock(&my_lock);
	
	add_wait_queue(array.structs[event_id].q, &wait2);
	while ((ret = array.structs[event_id].flag) == 0) {
		prepare_to_wait(array.structs[event_id].q, &wait2, TASK_INTERRUPTIBLE);
		schedule();
	}
	finish_wait(array.structs[event_id].q, &wait2);
	spin_lock(&my_lock);
	array.structs[event_id].num--;
	if (array.structs[event_id].flag == 2 && array.structs[event_id].num == 0) {
		kfree(array.structs[event_id].q);
		array.structs[event_id].q = NULL;
	}
	if (array.structs[event_id].flag == 1 && array.structs[event_id].num == 0) {
		array.structs[event_id].flag = 0;
		wake_up(array.structs[event_id].waking);
	}
	spin_unlock(&my_lock);
	if (ret == 1)
		return 0;
	return -EINVAL;
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
	if (acceleration == NULL) {
		return -EINVAL;
	}
	if (copy_from_user(&sensor_data, acceleration, sizeof(struct dev_acceleration)) != 0) {
		return -EINVAL;
	}
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
			difx = sensorDataBuffer[j].x - sensorDataBuffer[(j + 1) % WINDOW].x;
			dify = sensorDataBuffer[j].y - sensorDataBuffer[(j + 1) % WINDOW].y;
			difz = sensorDataBuffer[j].z - sensorDataBuffer[(j + 1) % WINDOW].z;
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
			array.structs[i].flag = 1;
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
	array.structs[event_id].flag = 2;
	wake_up(array.structs[event_id].q);
	wake_up(array.structs[event_id].waking);
	kfree(array.structs[event_id].motion);
	array.structs[event_id].motion = NULL;
	spin_unlock(&my_lock);
	return 0;
}


