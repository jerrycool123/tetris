#include "server.h"
Server::Server(const unsigned short _port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(hostname, sizeof(hostname));
	port = _port;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }


    maxfd = getdtablesize();
	player_queue = new queue[maxfd];
	for (int i = 0; i < maxfd; ++i) {
		player_queue[i].fd_isopen = false;
		player_queue[i].name_isvalid = false;
		memset(player_queue[i].name, 0, MAXNLEN);
	}
	player_count = 0;

	FD_ZERO (&master);
	FD_ZERO (&read_fds);
	FD_SET (listen_fd, &master);
	clilen = sizeof (cliaddr);
}
int Server::connection_establish() {
	clifd = accept(listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
	if (clifd < 0) {
		fprintf(stderr, "clifd = %d\n", clifd);
		if (errno == EINTR || errno == EAGAIN) return -1;  // try again
		if (errno == ENFILE) {
			(void) fprintf(stderr, "out of file descriptor table ... (maxfd %d)\n", maxfd);
			exit (1);
		}
		ERR_EXIT("accept");
	}
	fprintf(stderr, "getting a new request... fd %d from %s\n", clifd, inet_ntoa(cliaddr.sin_addr));
	FD_SET(clifd, &master);
	add_player(clifd);
	return clifd;
}
void Server::connection_close(int fd) {
	FD_CLR(fd, &master);
	fprintf(stderr, "connection of fd = %d is closed.\n", fd);
	close(fd);
	remove_player(fd);
	return;
}
int Server::handle_read(int fd, char *buf, size_t count) {
	int ret = read(fd, buf, count);
	fprintf(stderr, "receives %d bytes from fd = %d : %s\n", ret, fd, buf);
	if (ret < 0) return -1;
	else if (ret == 0) return 0;
	else {	// Avoid some abnormal input
		char* p1 = strstr(buf, "\015\012");
		int newline_len = 2;
		// be careful that in Windows, line ends with \015\012
		if (p1 == NULL) {
			p1 = strstr(buf, "\012");
			newline_len = 1;
			if (p1 == NULL) {
				return -1;
			}
		}
		size_t len = p1 - buf + 1;
		buf[--len] = '\0';
		for (int i = 0 ; i < len; ++i)	// Only permit alphabets and numbers
			if (!isalnum(buf[i]))
				return -2;
		if (len == 0)	// Empty input
			return -2;
		else return len;
	}
}
void Server::add_player(int fd) {
	if (player_queue[fd].fd_isopen == true) {
		fprintf(stderr, "ERROR: player_queue[%d] is nonzero.\n", fd);
		exit(1);
	}
	else {
		fprintf(stderr, "add player fd = %d.\n", fd);
		player_queue[fd].fd_isopen = true;
		player_queue[fd].name_isvalid = false;
		memset(player_queue[fd].name, 0, MAXNLEN);
		++player_count;
	}
	return;
}
void Server::remove_player(int fd) {
	if (player_queue[fd].fd_isopen == false) {
		fprintf(stderr, "ERROR: player fd = %d does not exist.\n", fd);
		exit(1);
	}
	else {
		fprintf(stderr, "remove player fd = %d.\n", fd);
		player_queue[fd].fd_isopen = false;
		player_queue[fd].name_isvalid = false;
		memset(player_queue[fd].name, 0, MAXNLEN);
		--player_count;
	}
	return;
}
void Server::get2players(int selected_players_fd[2]) {
	int j = 0;
	for (int i = 0; i < maxfd && j < 2; ++i) {
		if (player_queue[i].fd_isopen == true)
			selected_players_fd[j++] = i;
	}
	if (j != 2) {
		fprintf(stderr, "ERROR: can't select 2 players.\n");
		exit(1);
	}
	return;
}
int Server::name_inuse(char *name) {
	for (int i = 0; i < maxfd; ++i) {
		if (player_queue[i].name_isvalid == true && strncmp(player_queue[i].name, name, MAXNLEN) == 0)
			return 1;
	}
	return 0;
}
void Server::change_name(int fd, const char *name) {
	memset(player_queue[fd].name, 0, MAXNLEN);
	int nlen = strlen(name);
	if (nlen >= MAXNLEN) {
		fprintf(stderr, "ERROR: username is too long.\n");
		exit(1);
	}
	strncpy(player_queue[fd].name, name, nlen);
	player_queue[fd].name_isvalid = true;
	return;
}
