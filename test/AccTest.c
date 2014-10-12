#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
// #include <string.h>
// #include <errno.h>
#include "../flo-kernel/include/linux/acc_sync.h"

int launchOneDetector(int eventId);
#define MAX 20
int main(int argc, const char *argv[]) {

	// char *line = NULL;
	// size_t len = 0;
	// ssize_t n = 0;
	int numberOfDetectors;
	int eventQueue[MAX];
	unsigned int x, y, z, frq;
	struct acc_motion motion;
	FILE* input;
	if((input = fopen("input", "r"))!=NULL){
		fscanf(input, "%i\n", &numberOfDetectors);
		if(numberOfDetectors > MAX){
			numberOfDetectors = MAX;
		}
		int i;
		for(i = 0; i < numberOfDetectors; i++) {
			fscanf(input, "%u %u %u %u\n", &x, &y, &z, &frq);
			motion.dlt_x = x;
			motion.dlt_y = y;
			motion.dlt_z = z;
			motion.frq = frq;
			eventQueue[i] = syscall(379, &motion);
			// printf("%u %u %u %u\n", x, y, z, frq);
			launchOneDetector(eventQueue[i]);
		}
		sleep(70);
		for(i = 0; i < numberOfDetectors; i++) {
			syscall(382, eventQueue[i]);
		}
	}
	else {

	}
	fclose(input);
	return 0;
}

int launchOneDetector(int eventId) {
  pid_t pid = fork();
  //int status=1;
  // errno = 0;

  if (pid < 0) {
      exit(1);
  }
	if(pid == 0) {
				//int eventId;
				// struct acc_motion acceleration = {10,30,7, 9};
				/* Create an event based on motion. */
				//eventId = syscall(379, &motion);
				/* Block a process on an event. */
				syscall(380, eventId);
				printf("%d detected a shake\n", getpid());
				exit(0);
				// exit(syscall(380, eventId));
	}
	/*return value should = pid of child*/
	//while (wait(&status) != pid);

	//if(status == 0) {
	//	printf("%d detected a shake\n", pid);
	//}

	return 0;
}
