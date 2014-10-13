#ifndef _ACCELERATION_H
#define _ACCELERATION_H

extern struct dev_acceleration sensorData;

struct dev_acceleration {
	int x; /* acceleration along X-axis */
	int y; /* acceleration along Y-axis */
	int z; /* acceleration along Z-axis */
};

#endif
