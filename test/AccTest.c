#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <string.h>
#include <errno.h>
#include "../flo-kernel/include/linux/acc_sync.h"

int launchOneDetector();

int main(int argc, const char *argv[]) {

	// char *line = NULL;
	// size_t len = 0;
	// ssize_t n = 0;
	int N;
	FILE* input = fopen("input", "r");
	fscanf(input, "%i\n", &N);
	printf("%i",N);
	// for(int i = 0; i < N; i++) {
	// 	// n = getline(&line, &len, file);/*including newline character*/
	// 	launchOneDetector();
	// }

	return 0;
}

int launchOneDetector() {
  pid_t pid = fork();
  int status;
  errno = 0;

  if (pid < 0) {
      exit(1);
  }
	if(pid == 0) {
				// int eventId;
				// struct acc_motion acceleration = {10,30,7, 9};
				// /* Create an event based on motion. */
				// eventId = syscall(379, &acceleration);
				// /* Block a process on an event. */
				// syscall(380, eventId);
				exit(0);
	}
	// else {
		/*return value should = pid of child*/
	while (wait(&status) != pid);

	if(status == 0) {
		printf("%d detected a shake\n", pid);
	}
	// }

	return 0;
}
