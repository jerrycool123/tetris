#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define KEY_UP      0x0105
#define KEY_DOWN    0x0106
#define KEY_LEFT    0x0107
#define KEY_RIGHT   0x0108

class Keyboard {
	public:
		Keyboard(int _fd);
		int getch();
	private:
		int fd;
		struct termios term, oterm;
};
#endif
