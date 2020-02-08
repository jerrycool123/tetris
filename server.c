#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <sys/file.h>

#define ERR_EXIT(a) { perror(a); exit(1); }

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
	int account_id;		// corresponding account id 
	int account_balance;// corresponding account balance
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";
const char* modifiable_header = "This account is modifiable.\n";
const char* lock_header = "This account is locked.\n";
const char* fail_header = "Operation failed.\n";

// Forwards

void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

static int handle_read(request* reqP);
// return 0: socket ended, request done.
// return 1: success, message (without header) got this time is in reqP->buf with reqP->buf_len bytes. read more until got <= 0.
// It's guaranteed that the header would be correctly set after the first read.
// error code:
// -1: client connection error

static void connection_close (fd_set *master, int conn_fd);
// deal with connection close procedure, free some memory used

static int write_lock (int fd, int account_id);
// lock the read permission of a specific account

static int file_unlock (int fd, int account_id);
// file unlock

static int get_lock (int fd, int account_id);
// get locking status

static int get_account_balance (int fd, int account_id);
// return -1: the account is locked
// otherwise: the balance of the account

static void modify_account_balance (int fd, int account_id, int account_balance);
// change the balance of a specific account

int main(int argc, char** argv) {
    int i, ret;

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd;  // fd for file that we open for reading
    char buf[512];
    int buf_len;
	
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

	// Initailize account_list
	file_fd = open ("account_list", O_RDWR)	;
	if (file_fd < 0) {
		ERR_EXIT ("open");
	}

    // Get file descripter table size and initize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

	fd_set read_fds, master;	// read_fds: temp set for select function; master: main set
	FD_ZERO (&master);
	FD_ZERO (&read_fds);
	FD_SET (svr.listen_fd, &master);
	int select_ret;

    while (1) {
        // TODO: Add IO multiplexing
		read_fds = master;
		select_ret = select (maxfd, &read_fds, NULL, NULL, &tv);
		if (select_ret == -1) {
			ERR_EXIT ("select");
		}
		for (int i = 0; i < maxfd; i++) {
			if (FD_ISSET (i, &read_fds)) {
				if (i == svr.listen_fd) {	// Check new connection
					clilen = sizeof(cliaddr);
					conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
					if (conn_fd < 0) {
						if (errno == EINTR || errno == EAGAIN) continue;  // try again
						if (errno == ENFILE) {
							(void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
							exit (1);
						}
						ERR_EXIT("accept")
					}
					requestP[conn_fd].conn_fd = conn_fd;
					strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
					fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
					FD_SET (conn_fd, &master);
				}
				else {	//	Handle data from a client
					conn_fd = i;
					ret = handle_read(&requestP[conn_fd]); // parse data from client to requestP[conn_fd].buf
					if (ret < 0) {
						fprintf(stderr, "bad request from %s\n", requestP[conn_fd].host);
						connection_close (&master, conn_fd);
						continue;
					}

					int account_id;
					int account_balance, account_balance2;
					char arg[3][16] = {};	// format string input array
					int strnum, amount1, amount2;

#ifdef READ_SERVER
					account_id = atoi (requestP[conn_fd].buf);
					if (!(account_id >= 1 && account_id <= 20)) {
						fprintf (stderr, "the account %s doesn't exist.\n", requestP[conn_fd].buf);
						connection_close (&master, conn_fd);
						continue;
					}
					account_balance = get_account_balance (file_fd, account_id);
					if (account_balance < 0) {	// the account is locked
						write(requestP[conn_fd].conn_fd, lock_header, strlen(lock_header));
					}
					else {
						sprintf (buf, "%d %d\n", account_id, account_balance);
						write(requestP[conn_fd].conn_fd, buf, strlen(buf));
					}
					connection_close (&master, conn_fd);
#else
					if (requestP[conn_fd].wait_for_write == 0) {	// first session of input
						account_id = atoi (requestP[conn_fd].buf);
						if (!(account_id >= 1 && account_id <= 20)) {
							fprintf (stderr, "the account %s doesn't exist.\n", requestP[conn_fd].buf);
							connection_close (&master, conn_fd);
							continue;
						}
						account_balance = get_account_balance (file_fd, account_id);
						for (int j = 0; account_balance >= 0 && j < maxfd; j++)	{	// check if the account is under modifying by other request
							if (requestP[j].account_id == account_id) {
								account_balance = -1;
							}
						}
						if (account_balance < 0) {	// the account is locked
							write(requestP[conn_fd].conn_fd, lock_header, strlen(lock_header));
							connection_close (&master, conn_fd);
							continue;
						}
						else {
							write(requestP[conn_fd].conn_fd, modifiable_header, strlen(modifiable_header));
							write_lock (file_fd, account_id);	// lock the account
							requestP[conn_fd].account_id = account_id;
							requestP[conn_fd].account_balance = account_balance;
							requestP[conn_fd].wait_for_write = 1;
							continue;
						}
					}
					else {
						account_id = requestP[conn_fd].account_id;
						account_balance = requestP[conn_fd].account_balance;
						strcpy (buf, requestP[conn_fd].buf);
						strnum = sscanf (buf, "%s%s%s", arg[0], arg[1], arg[2]);	// format string input
						if (strnum == 2 && !strcmp (arg[0], "save")) {
							amount1 = atoi (arg[1]);
							if (amount1 >= 0) {	// insure save amount >= 0
								account_balance += amount1;
								modify_account_balance (file_fd, account_id, account_balance);
							}
							else {
								write(requestP[conn_fd].conn_fd, fail_header, strlen(fail_header));
							}
						}
						else if (strnum == 2 && !strcmp (arg[0], "withdraw")) {
							amount1 = atoi (arg[1]);
							if (amount1 >= 0 && amount1 <= account_balance) {	// insure withdraw amount >= 0 and <= balance
								account_balance -= amount1;
								modify_account_balance (file_fd, account_id, account_balance);
							}
							else {
								write(requestP[conn_fd].conn_fd, fail_header, strlen(fail_header));
							}
						}
						else if (strnum == 3 && !strcmp (arg[0], "transfer")) {
							amount1 = atoi (arg[1]);
							amount2 = atoi (arg[2]);
							account_balance2 = get_account_balance (file_fd, amount1);
							if (account_balance2 < 0) {	// the 2nd account is locked
								write(requestP[conn_fd].conn_fd, lock_header, strlen(lock_header));
							}
							else if (amount2 >= 0 && amount2 <= account_balance) {	// insure withdraw amount >= 0 and <= balance
								account_balance -= amount2;
								account_balance2 += amount2;
								modify_account_balance (file_fd, account_id, account_balance);
								modify_account_balance (file_fd, amount1, account_balance2);
							}
							else {
								write(requestP[conn_fd].conn_fd, fail_header, strlen(fail_header));
							}
						}
						else if (strnum == 2 && !strcmp (arg[0], "balance")) {
							amount1 = atoi (arg[1]);
							if (amount1 >= 0) {	// insure new amount >= 0
								account_balance = amount1;
								modify_account_balance (file_fd, account_id, account_balance);
							}
							else {
								write(requestP[conn_fd].conn_fd, fail_header, strlen(fail_header));
							}
						}
						else {
							write(requestP[conn_fd].conn_fd, fail_header, strlen(fail_header));
						}
						file_unlock (file_fd, account_id);	// unlock the account
					}
					connection_close (&master, conn_fd);
#endif
    			}
			}
		}
	}		
	free (requestP);
    return 0;
}


// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void* e_malloc(size_t size);


static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->account_id = -1;
	reqP->account_balance = -1;
    reqP->wait_for_write = 0;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

// return 0: socket ended, request done.
// return 1: success, message (without header) got this time is in reqP->buf with reqP->buf_len bytes. read more until got <= 0.
// It's guaranteed that the header would be correctly set after the first read.
// error code:
// -1: client connection error
static int handle_read(request* reqP) {
    int r;
    char buf[512];

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
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
	memmove(reqP->buf, buf, len);
	reqP->buf[len - 1] = '\0';
	reqP->buf_len = len-1;
    return 1;
}

void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }
}

static void* e_malloc(size_t size) {
    void* ptr;

    ptr = malloc(size);
    if (ptr == NULL) ERR_EXIT("out of memory");
    return ptr;
}

static void connection_close (fd_set *master, int conn_fd) {
	FD_CLR (requestP[conn_fd].conn_fd, master);
	close(requestP[conn_fd].conn_fd);
	free_request(&requestP[conn_fd]);
}

static int write_lock (int fd, int account_id) {
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_start = sizeof (int) * ((account_id - 1) * 2) ;
	lock.l_whence = SEEK_SET;
	lock.l_len = sizeof (int) * 2;
	lock.l_pid = getpid ();
	if (fcntl (fd, F_SETLK, &lock) == 0)
		return 1;
	else
		return 0;
}

static int file_unlock (int fd, int account_id) {
	struct flock lock;
	lock.l_type = F_UNLCK;
	lock.l_start = sizeof (int) * ((account_id - 1) * 2);
	lock.l_whence = SEEK_SET;
	lock.l_len = sizeof (int) * 2;
	lock.l_pid = getpid ();
	if (fcntl (fd, F_SETLKW, &lock) == 0)
		return 1;
	else
		return 0;

}

static int get_lock (int fd, int account_id) {
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_start = sizeof (int) * ((account_id - 1) * 2);
	lock.l_whence = SEEK_SET;
	lock.l_len = sizeof (int) * 2;
	lock.l_pid = getpid ();
/*	fprintf (stderr, "%d\n", lock.l_type);
	fprintf (stderr, "%d\n", lock.l_start);
	fprintf (stderr, "%d\n", lock.l_len);
	fprintf (stderr, "%d\n", lock.l_pid);*/
	fcntl (fd, F_GETLK, &lock);
/*	fprintf (stderr, "fcntl:\n");
	fprintf (stderr, "%d\n", lock.l_type);
	fprintf (stderr, "%d\n", lock.l_start);
	fprintf (stderr, "%d\n", lock.l_len);
	fprintf (stderr, "%d\n", lock.l_pid);*/
	if (lock.l_type == F_UNLCK)
		return 0;
	else if (lock.l_type == F_WRLCK)
		return 1;
	else {
		fprintf (stderr, "lock error.\n");
		exit (1);
	}
}

static int get_account_balance (int fd, int account_id) {
	int is_locked = get_lock (fd, account_id);
	if (is_locked)
		return -1;
	else {
		int balance;
		lseek (fd, sizeof (int) * (account_id * 2 - 1), SEEK_SET);
		read (fd, &balance, sizeof (int));
		return balance;
	}
}

static void modify_account_balance (int fd, int account_id, int account_balance) {
	lseek (fd, sizeof (int) * (account_id * 2 - 1), SEEK_SET);
	write (fd, &account_balance, sizeof (int));
}
