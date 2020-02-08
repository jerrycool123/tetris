#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#define print printf("%s",c)
#define printg printf("%s",d)
#define endl printf("\n")

int main() {
	const char *d = "█";
	const char *c = "#";
	int j = 0;
	while(1) {
		usleep(1*1000000);
		system("clear");
		for (int i = 0; i < 21; ++i) {
			for (int j = 0; j < 36; ++j) {
				print;print;print;
			}
			endl;
			for (int j = 0; j < 36; ++j) {
				print;print;print;
			}
			endl;
		}
	//	puts("     player ABCDEFGHIJKLMNO");
		puts("     player Jerry");
	}
	/*
	print;print;print;
	endl;
	print;print;print;
	endl;
	print;print;print;print;print;print;print;print;print;
	endl;
	print;print;print;print;print;print;print;print;print;
	endl;
	*/
}
