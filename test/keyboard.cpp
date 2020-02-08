#include "keyboard.h"

Keyboard::Keyboard(int _fd) {
	fd = _fd;
}

int Keyboard::getch()
{
    int c = 0;

    tcgetattr(fd, &oterm);
    memcpy(&term, &oterm, sizeof(term));
    term.c_lflag &= ~(ICANON | ECHO);
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
    tcsetattr(fd, TCSANOW, &term);
 	read(fd, &c, 1);	// remember to fflush(stdout)
	//   c = getchar();
    tcsetattr(fd, TCSANOW, &oterm);
    return c;
}
