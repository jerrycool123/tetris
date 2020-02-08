#include <stdio.h>
#include "keyboard.h"

int main() {
	Keyboard kb(0);
	char c;
	while (c = kb.getch()) {
		printf("%d\n", c);
	}
}
