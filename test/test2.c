#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

int main() {
	char c;
	printf("len = %d\n", strlen("\x1B[31m"));
	return 0;
	while (1) {
		c = getchar();
		printf("c = %d\n", c);
	}
}

