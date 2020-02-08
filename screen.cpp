#include "screen.h"

char Screen::clear_str[16] = "\033[2J\033[1;1H";  
char Screen::no_echo_mode_str[16] = "\377\374\001"; 
char Screen::echo_mode_str[16] = "\377\373\001"; 
char Screen::line_mode_str[16] = "\377\375\042"; 

void Screen::clear(int fd, int *write_ret) {
	*write_ret = write(fd, clear_str, strlen(clear_str));
	return;
}
void Screen::set_no_echo_mode(int fd, int *write_ret) {
	*write_ret = write(fd, no_echo_mode_str, strlen(no_echo_mode_str));
	return;
}
void Screen::set_echo_mode(int fd, int *write_ret) {
	*write_ret = write(fd, echo_mode_str, strlen(echo_mode_str));
	return;
}
void Screen::set_line_mode(int fd, int *write_ret) {
	*write_ret = write(fd, line_mode_str, strlen(line_mode_str));
	return;
}
void Screen::append_gotorc(char *buf, int *len, int r, int c) {
	char local_buf[16] = "";
	sprintf(local_buf, "\033[%d;%df", r, c);
	strcpy(buf + *len, local_buf);
	*len += strlen(local_buf);
	return;
}
