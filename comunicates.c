#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <poll.h>


void *output(void *arg){
	int sock = *(int *)(arg);
	char in[1024];
	
	for(;;){
		fputs("私:", stdout);
		fgets(in, sizeof in, stdin);
		write(sock, in, sizeof in);
		puts("送信しました");
	}

	exit(0);
}

void *input(void *arg){
	int sock = *(int *)arg, nfds;
	char in[1024];
	struct pollfd fds[1];

	for(;;){
		fds[0].fd = sock;
		fds[0].events = POLLRDHUP;
		nfds = poll(fds, 1, 0);
		if(nfds == -1){
			fputs("poll: err\n", stderr);
			exit(1);
		}
		if(fds[0].revents & POLLRDHUP){
			puts("ぴあソケットがクローズされました");
			exit(1);
		}

		read(sock, in, sizeof in);
		printf("\n相手:%s", in);
		fputs("私:", stdout);
	}
	exit(0);
}
