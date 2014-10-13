#ifndef _ACC_SYNC_H
#define _ACC_SYNC_H

#define WINDOW 20

extern spinlock_t my_lock;
extern struct motionArray array;
extern struct dev_acceleration sensorDataBuffer[WINDOW];
extern int sensorDataBufferHead;

#endif
