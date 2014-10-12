/*
 * Columbia University
 * COMS W4118 Fall 2014
 * Homework 3
 *
 */
#include <bionic/errno.h> /* Google does things a little different...*/
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <hardware/hardware.h>
#include <hardware/sensors.h> /* <-- This is a good place to look! */
#include "../flo-kernel/include/linux/akm8975.h"
#include "acceleration.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>
#include <sys/syscall.h>

/* from sensors.c */
#define ID_ACCELERATION   (0)
#define ID_MAGNETIC_FIELD (1)
#define ID_ORIENTATION	  (2)
#define ID_TEMPERATURE	  (3)

#define SENSORS_ACCELERATION   (1<<ID_ACCELERATION)
#define SENSORS_MAGNETIC_FIELD (1<<ID_MAGNETIC_FIELD)
#define SENSORS_ORIENTATION    (1<<ID_ORIENTATION)
#define SENSORS_TEMPERATURE    (1<<ID_TEMPERATURE)

#define TIME_INTERVAL 100
#define SENSOR_DELAY 100

/* set to 1 for a bit of debug output */
#if 1
	#define dbg(fmt, ...) printf("Accelerometer: " fmt, ## __VA_ARGS__)
#else
	#define dbg(fmt, ...)
#endif

static int effective_sensor;

/* helper functions which you should use */
static int open_sensors(struct sensors_module_t **hw_module,
			struct sensors_poll_device_t **poll_device);
static void enumerate_sensors(const struct sensors_module_t *sensors);

static int poll_sensor_data(struct sensors_poll_device_t *sensors_device)
{
	const size_t numEventMax = 16;
	const size_t minBufferSize = numEventMax;
	sensors_event_t buffer[minBufferSize];
	ssize_t count = sensors_device->poll(sensors_device, buffer, minBufferSize);
	int i;

	struct dev_acceleration acc;
	for (i = 0; i < count; ++i) {
		if (buffer[i].sensor != effective_sensor)
			continue;

		/* At this point we should have valid data*/
		/* Scale it and pass it to kernel*/

		acc.x = 100 * buffer[i].acceleration.x;
		acc.y = 100 * buffer[i].acceleration.y;
		acc.z = 100 * buffer[i].acceleration.z;
		syscall(381, &acc);
		dbg("Acceleration: x= %0.2f, y= %0.2f, "
			"z= %0.2f\n", buffer[i].acceleration.x,
			buffer[i].acceleration.y, buffer[i].acceleration.z);

	}
	return 0;
}

/* entry point: fill in daemon implementation
   where indicated */
int main(int argc, char **argv)
{
	/*Citation: http://www.netzmafia.de/skripten/unix/linux-daemon-howto.html*/
	pid_t pid, sid;

	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}
	umask(0);
	sid = setsid();
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}
	if ((chdir("/")) < 0) {
		exit(EXIT_FAILURE);
	}
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	
	effective_sensor = -1;
	struct sensors_module_t *sensors_module = NULL;
	struct sensors_poll_device_t *sensors_device = NULL;

	printf("Opening sensors...\n");
	if (open_sensors(&sensors_module,
			 &sensors_device) < 0) {
		printf("open_sensors failed\n");
		return EXIT_FAILURE;
	}
	enumerate_sensors(sensors_module);

	struct acc_motion motion1 = {3000, 0, 0, 10};
	struct acc_motion motion2 = {3000, 0, 0, 10};
	struct acc_motion motion3 = {3000, 0, 0, 10};
	struct acc_motion motion4 = {3000, 0, 0, 10};
	struct acc_motion motion5 = {3000, 0, 0, 10};
	struct acc_motion motion6 = {0, 3000, 0, 10};
	struct acc_motion motion7 = {0, 3000, 0, 10};
	struct acc_motion motion8 = {0, 3000, 0, 10};
	struct acc_motion motion9 = {0, 3000, 0, 10};
	struct acc_motion motion10 = {0, 3000, 0, 10};
	struct acc_motion motion11 = {0, 0, 3000, 10};
	struct acc_motion motion12 = {0, 0, 3000, 10};
	struct acc_motion motion13 = {0, 0, 3000, 10};
	struct acc_motion motion14 = {0, 0, 3000, 10};
	struct acc_motion motion15 = {0, 0, 3000, 10};

	syscall(379, &motion1);	
	syscall(379, &motion2);	
	syscall(379, &motion3);	
	syscall(379, &motion4);	
	syscall(379, &motion5);	
	syscall(379, &motion6);	
	syscall(379, &motion7);	
	syscall(379, &motion8);	
	syscall(379, &motion9);	
	syscall(379, &motion10);	
	syscall(379, &motion11);	
	syscall(379, &motion12);	
	syscall(379, &motion13);	
	syscall(379, &motion14);	
	syscall(379, &motion15);	


	while (1) {
		/* Do some task here ... */
		poll_sensor_data(sensors_device);  
		usleep(TIME_INTERVAL);
	}
	return EXIT_SUCCESS;
}

/*                DO NOT MODIFY BELOW THIS LINE                    */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int open_sensors(struct sensors_module_t **mSensorModule,
			struct sensors_poll_device_t **mSensorDevice)
{
   
	int err = hw_get_module(SENSORS_HARDWARE_MODULE_ID,
				     (hw_module_t const**)mSensorModule);

	if (err) {
		printf("couldn't load %s module (%s)",
			SENSORS_HARDWARE_MODULE_ID, strerror(-err));
	}

	if (!*mSensorModule)
		return -1;

	err = sensors_open(&((*mSensorModule)->common), mSensorDevice);

	if (err) {
		printf("couldn't open device for module %s (%s)",
			SENSORS_HARDWARE_MODULE_ID, strerror(-err));
	}

	if (!*mSensorDevice)
		return -1;

	const struct sensor_t *list;
	ssize_t count = (*mSensorModule)->get_sensors_list(*mSensorModule, &list);
	size_t i;
	for (i=0 ; i<(size_t)count ; i++) {
		(*mSensorDevice)->setDelay(*mSensorDevice, list[i].handle, SENSOR_DELAY);
		(*mSensorDevice)->activate(*mSensorDevice, list[i].handle, 1);
	}
	return 0;
}

static void enumerate_sensors(const struct sensors_module_t *sensors)
{
	int nr, s;
	const struct sensor_t *slist = NULL;
	if (!sensors)
		printf("going to fail\n");

	nr = sensors->get_sensors_list((struct sensors_module_t *)sensors,
					&slist);
	if (nr < 1 || slist == NULL) {
		printf("no sensors!\n");
		return;
	}

	for (s = 0; s < nr; s++) {
		printf("%s (%s) v%d\n\tHandle:%d, type:%d, max:%0.2f, "
			"resolution:%0.2f \n", slist[s].name, slist[s].vendor,
			slist[s].version, slist[s].handle, slist[s].type,
			slist[s].maxRange, slist[s].resolution);

		/* Awful hack to make it work on emulator */
		if (slist[s].type == 1 && slist[s].handle == 0)
			effective_sensor = 0; /*the sensor ID*/

                }
}
