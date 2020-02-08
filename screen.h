#include <string.h>
#include <stdio.h>
#include <unistd.h>
#ifndef _SCREEN_H
#define _SCREEN_H
class Screen {
	public:
		static void clear(int fd, int *write_ret);	// Clear fd's screen
		static void set_no_echo_mode(int fd, int *write_ret);	// Set fd into no-echo-mode (telnet protocol)
		static void set_echo_mode(int fd, int *write_ret);	// Set fd into echo-mode (telnet protocol)
		static void set_line_mode(int fd, int *write_ret);	// Set fd into line-mode (telnet protocol)
		static void append_gotorc(char *buf, int *len, int r, int c);	// Append the following instruction to buf: Move the cursor to (r, c), where the upperleft corner is (1, 1)
		static char clear_str[16];  
		static char no_echo_mode_str[16]; 
		static char echo_mode_str[16]; 
		static char line_mode_str[16]; 
};
#endif
