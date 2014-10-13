#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include "../flo-kernel/include/linux/acc_motion.h"

#define MAX 20

int launchOneDetector(int eventId, struct acc_motion motion)
{
	pid_t pid = fork();

	if (pid < 0)
		exit(1);
	if (pid == 0) {
		/* Block a process on an event. */
		if (syscall(380, eventId) == 0) {
			if (motion.dlt_x != 0 && motion.dlt_y == 0
				&& motion.dlt_z == 0) {
				printf("%d detected a horizontal shake\n",
					getpid());
				printf("x=%u,y=%u,z=%u,f=%u\n", motion.dlt_x,
					motion.dlt_y, motion.dlt_z, motion.frq);
			} else if (motion.dlt_x == 0 && motion.dlt_y != 0
				&& motion.dlt_z == 0) {
				printf("%d detected a vertical shake\n",
					getpid());
				printf("x=%u,y=%u,z=%u,f=%u\n", motion.dlt_x,
					motion.dlt_y, motion.dlt_z, motion.frq);
			} else {
				printf("%d detected a shake\n", getpid());
				printf("x=%u,y=%u,z=%u,f=%u\n", motion.dlt_x,
					motion.dlt_y, motion.dlt_z, motion.frq);
			}
		}
		exit(0);
		/* exit(syscall(380, eventId)); */
	}
	return 0;
}
int main(int argc, const char *argv[])
{
	int numberOfDetectors;
	int eventQueue[MAX];
	unsigned int x, y, z, frq;
	struct acc_motion motion;
	FILE *input;
	int i;

	input = fopen("input", "r");
	if (input != NULL) {
		fscanf(input, "%i\n", &numberOfDetectors);
		if (numberOfDetectors > MAX)
			numberOfDetectors = MAX;
		for (i = 0; i < numberOfDetectors; i++) {
			fscanf(input, "%u %u %u %u\n", &x, &y, &z, &frq);
			motion.dlt_x = x;
			motion.dlt_y = y;
			motion.dlt_z = z;
			motion.frq = frq;
			/* Create an event based on motion. */
			eventQueue[i] = syscall(379, &motion);
			launchOneDetector(eventQueue[i], motion);
		}
		sleep(65);
		for (i = 0; i < numberOfDetectors; i++) {
			/* Destroy an acceleration event using the event_id. */
			syscall(382, eventQueue[i]);
		}
	} else {
		printf("No Input File!\n");
	}
	fclose(input);
	return 0;
}
