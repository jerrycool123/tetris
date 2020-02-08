#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("usage: %s [pid]\n", argv[0]);
		exit(1);
	}
	pid_t pid = atoi(argv[1]);
	printf("pid = %d, ppid = %d\n", pid, getppid());
	return 0;
}
