#include <stdio.h>
#include <unistd.h>
#include "keyboard.h"

int main() {
	Keyboard kb_obj(STDIN_FILENO);
	char c;
    while (1) {
        c = kb_obj.getch();
        if (c == KEY_UP) {
			printf("up\n");
        } else
        if (c == KEY_DOWN) {
			printf("down\n");
        } else
        if (c == KEY_RIGHT) {
			printf("right\n");
        } else
        if (c == KEY_LEFT) {
			printf("left\n");
        } else {
            putchar(c);
        }
			fflush(stdout);
    }
	return 0;
}
