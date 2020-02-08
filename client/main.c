// reference: http://zake7749.github.io/2015/03/17/SocketProgramming/#Client__u7BC4_u4F8B_u7A0B_u5F0F
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define ERR_EXIT(a) { perror(a); exit(1); }

const char *name_header = "name=";
const char *invalid_msg = "Invalid Username.\n";

int main(int argc , char *argv[])
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s [IPv4 address] [port]\n", argv[0]);
		exit(1);
	}
    // socket establishment
    int sockfd = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) ERR_EXIT("socket");


    // socket connection

    struct sockaddr_in info;
    bzero(&info, sizeof(info));
    info.sin_family = AF_INET;
    info.sin_addr.s_addr = inet_addr(argv[1]);
    info.sin_port = htons(atoi(argv[2]));


    int err = connect(sockfd, (struct sockaddr *)&info, sizeof(info));
    if(err == -1){
		ERR_EXIT("connect");
    }

	char buf[1024] = "", username[1024] = "";

	read(sockfd, buf, sizeof (buf));
	printf("%s", buf);
	do {
		memset(buf, 0, sizeof (buf));
		memset(username, 0, sizeof (username));
		scanf("%s", username);
		usleep(0.5*1000000);
		sprintf(buf, "%s%s", name_header, username);

		write(sockfd, buf, strlen(buf));
		memset(buf, 0, sizeof (buf));
		read(sockfd, buf, sizeof (buf));
		printf("%s", buf);
	} while (strncmp(buf, invalid_msg, strlen(invalid_msg)) == 0);
    return 0;
}
