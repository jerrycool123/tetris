#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include "keyboard.h"
#include "server.h"
#include "match.h"
#include "screen.h"

#define rule_filename "rule.txt"
#define error_log_path "./error_log/"

const char *invalid_msg = "\nInvalid Username.\n";
const char *inuse_msg = "\nThis name is already used by another player!\n";
const char *welcome_msg = "- welcome, ";
const char *wait_msg = "!\n- Waiting for another player to begin a new match...\n";

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

	Server server(atoi(argv[1]));	// Init monitor server
    fprintf(stderr, "starting on %.80s, port %d, fd %d, maxconn %d...\n", server.hostname, server.port, server.listen_fd, server.maxfd);

	int clifd;	// Cilent file descriptor
	char buf[1024] = "";	// IO buffer
	char username[16] = "", *nameptr[2];
	int read_bytes;	
	/*	 Warning: It does not count the exact bytes received
		 -1					: Illegal character that is threatening, may leading server crash.
		 -2					: Illegal character that isn't threatening, which may be ignored under some scenario.
		 positive interger n: Successfully read n legal characters.		*/
	int pid;
	int spfd[2];	// Selected players' file descritor
	int select_ret, write_ret;
	int is_continue;

	struct timeval select_tv;	// For non-blocking select()
	
	// Load welcome message
	char rule_msg[2048] = "";
	FILE *fp = fopen(rule_filename, "rb");
	if (fp == NULL) {
		ERR_EXIT("main fopen:");
	}
	fread(rule_msg, sizeof (rule_msg), 1, fp);
	fclose(fp);

	while (1) {		// The monitor server use select() for I/O Multiplexing
		server.read_fds = server.master;
		fprintf(stderr, "select function is now pending...\n");
		select_ret = select (server.maxfd, &server.read_fds, NULL, NULL, NULL);
		if (select_ret == -1) {
			ERR_EXIT ("select");
		}
		for (int fd_index = 0; fd_index < server.maxfd; ++fd_index) {
			if (FD_ISSET (fd_index, &server.read_fds)) {
				if (fd_index == server.listen_fd) {	// Check new connection
					clifd = server.connection_establish();
					if (clifd == -1)
						continue;
					Screen::clear(clifd, &write_ret);
					write(clifd, rule_msg, strlen(rule_msg));
					write(clifd, SET_DECCKM, strlen(SET_DECCKM));
				}
				else {	// Receives client data
					memset(buf, 0, sizeof (buf));	// Avoid dummy bytes
					read_bytes = server.handle_read(fd_index, buf, sizeof (buf));
					fprintf(stderr, "read legal bytes = %d from fd = %d\n", read_bytes, fd_index);
					if (server.player_queue[fd_index].name_isvalid == true) {
						if (read_bytes == -1) {	// Illegal_input
							fprintf(stderr, "case \"illegal input\"\n");
							server.connection_close(fd_index);
						}
						else {
							fprintf(stderr, "case \"already valid\"\n");
							// Do nothing
						}
					}
					else if (read_bytes < 0) {	// Illegal input
						fprintf(stderr, "case \"illegal input\"\n");
						write(fd_index, invalid_msg, strlen(invalid_msg));
						server.connection_close(fd_index);
					}
					else if (read_bytes == 0) {	// Client connection closed
						fprintf(stderr, "case \"zero byte\"\n");
						server.connection_close(fd_index);
					}
					else {	// Read dummy message
						fprintf(stderr, "fd = %d, name_isvalid = %d\n", fd_index, server.player_queue[fd_index].name_isvalid);
						memset(username, 0, sizeof (username));
						strncpy(username, buf, (sizeof (username) - 1));
						if (server.name_inuse(username) == 1) {
							write(fd_index, inuse_msg, strlen(inuse_msg));
							server.connection_close(fd_index);
						}
						else {
							server.change_name(fd_index, username);
							Screen::clear(fd_index, &write_ret);
							sprintf(buf, "%s%s%s", welcome_msg, username, wait_msg);
							fprintf(stderr, "buflen = %d, buf = %s\n", strlen(buf), buf);
							write(fd_index, buf, strlen(buf));
						}
					}
				}
			}
		}
		if (server.player_count >= 2) {
			server.get2players(spfd);	// Distribute every 2 players to a new match 
			is_continue = 0;
			for (int i = 0; i < 2 && !is_continue; ++i) {
				if (server.player_queue[spfd[i]].name_isvalid == false)
					is_continue = 1;
			}
			if (is_continue) continue;
			if ((pid = fork()) == 0) {	// Double fork to avoid zombie process
				if (fork() == 0) {	// Grandchild process deals with the new match 
					FD_CLR(spfd[0], &server.master);
					FD_CLR(spfd[1], &server.master);
					for (int fd_index = 3; fd_index < server.maxfd; ++fd_index) {	// Close redundant fds
						if (FD_ISSET(fd_index, &server.master)) {
							close(fd_index);	// Avoid the socket receives new connections
							FD_CLR(fd_index, &server.master);
						}
					}

					FD_ZERO(&server.master);
					FD_ZERO(&server.read_fds);
					FD_SET(spfd[0], &server.master);
					FD_SET(spfd[1], &server.master);
					nameptr[0] = server.player_queue[spfd[0]].name;
					nameptr[1] = server.player_queue[spfd[1]].name;

					// Build Error Log
					char error_log_filename[32] = "";
					sprintf(error_log_filename, "%s%s_vs_%s", error_log_path, nameptr[0], nameptr[1]);
					int error_log_fd = open(error_log_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);

					// stderr >> error log
					dup2(error_log_fd, STDERR_FILENO);

					Match match(spfd, nameptr);	// Start a new match
					
					// Init select() timeval = 0 (non-blocking select())
					select_tv.tv_sec = 0;
					select_tv.tv_usec = 0;
					

					while (match.ended == false) {	// The match use non-blocking select() for I/O Multiplexing and timeout detection
						// Potential problem: Select() will return frequently, and thus may waste some resources
						server.read_fds = server.master;
						select_ret = select (server.maxfd, &server.read_fds, NULL, NULL, &select_tv);
						
						match.check_timeout();	// If timeout, perform some match behaviors
						
						if (select_ret == -1) {
							//continue;
							fprintf(stderr, "%d\n", errno);
							ERR_EXIT ("select");
						}
						// Receive data from either clients
						if (match.fd_isopen[0] == true && FD_ISSET(spfd[0], &server.read_fds)) {
							memset(buf, 0, sizeof (buf));	// Avoid dummy bytes
							read(spfd[0], buf, sizeof (buf));
							match.trans_instruction(0, buf);
							if (match.fd_isopen[0] == false) {	// Check_connection
								FD_CLR(spfd[0], &server.master);
							}
						}
						if (match.fd_isopen[1] == true && FD_ISSET(spfd[1], &server.read_fds)) {
							memset(buf, 0, sizeof (buf));	// Avoid dummy bytes
							read(spfd[1], buf, sizeof (buf));
							match.trans_instruction(1, buf);
							if (match.fd_isopen[1] == false) {	// Check_connection
								FD_CLR(spfd[1], &server.master);
							}
						}
						if (match.fd_isopen[0] == false && match.fd_isopen[1] == false) {	// Both players left the match unexpectedly
							match.ended = true;
						}
						if (match.player_freezed[0] == true && match.board[0].score < match.board[1].score) {
							match.win(1);
						}
						else if (match.player_freezed[1] == true && match.board[0].score > match.board[1].score) {
							match.win(0);
						}
						else if (match.player_freezed[0] == true && match.player_freezed[1] == true) {
							if (match.board[0].score > match.board[1].score) {
								match.win(0);
							}
							else if (match.board[0].score < match.board[1].score) {
								match.win(1);
							}
							else if (match.board[0].score == match.board[1].score) {
								match.win(2);
							}
						}
					}
					fprintf(stderr, "match ended\n");
					match.match_close();	// Match finished
					close(error_log_fd);	// Close error log fd
					exit(0);	// Process termination
				}
				else {	// Child process just exit, and then the remaining zombie process will be cleaned by parent process waitpid(), with the grandchild process becoming an orphan process.
					exit(0);
				}
			}
			else {
				waitpid(pid, NULL, 0);
				for (int i = 0; i < 2; ++i)
					server.connection_close(spfd[i]);
				continue;
			}
		}
	}
	printf("END\n");
	return 0;
}
