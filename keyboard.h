#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#define KEY_UP      "\033\133\101"
#define KEY_DOWN    "\033\133\102"
#define KEY_LEFT    "\033\133\104"
#define KEY_RIGHT   "\033\133\103"
// DECCKM: https://vt100.net/docs/vt510-rm/DECCKM
#define SET_DECCKM	"\033[?1l"
#define KEY_Z		"z"
#define KEY_X		"x"
#define KEY_R		"r"
#define KEY_CTRL_C 	"\003"
#define KEY_CTRL_Z 	"\032"
#define KEY_CTRL_backslash "\034"
#define KEY_CAT     "c"

class Keyboard {
	public:
		Keyboard();	// Warning: Still need fd initialization! (set for some compatible reason)
		Keyboard(int _fd);
		void set_fd(int _fd);
		int getch();
	private:
		int fd;
		FILE *fp;
		struct termios term, oterm;
};
#endif
