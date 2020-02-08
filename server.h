#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#ifndef _SERVER_H
#define _SERVER_H


#define ERR_EXIT(a) { perror(a); exit(1); }
#define MAXNLEN 16

struct queue {
	bool fd_isopen;
	bool name_isvalid;
	char name[MAXNLEN];
};

class Server {
	public:
		Server(const unsigned short _port);	// Init server
		int connection_establish();
		void connection_close(int fd);
		int handle_read(int fd, char *buf, size_t count);
		void add_player(int fd);
		void remove_player(int fd);
		char hostname[512];  // server's hostname
		unsigned short port;  // port to listen
		int listen_fd;  // fd to wait for a new connection
		int maxfd;
		queue *player_queue;
		int player_count;
		void get2players(int selected_players_fd[2]);
		int name_inuse(char *name);
		void change_name(int fd, const char *name);
		fd_set read_fds, master;	// read_fds: temp set for select function; master: main set
	private:
		struct sockaddr_in cliaddr;	// used by accept()
		int clilen, clifd;
};
#endif
