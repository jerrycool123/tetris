#include "keyboard.h"

Keyboard::Keyboard() {
	// Do nothing
}

Keyboard::Keyboard(int _fd) {
	set_fd(_fd);
}

void Keyboard::set_fd(int _fd) {
	fd = _fd;
}

int Keyboard::getch()
{
    int c = 0;
	read(fd, &c, 1);
	return c;
}
