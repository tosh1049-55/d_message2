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
#include <ncurses.h>
#include <pthread.h>

void *input(void *arg);
void *output(void *arg);

int message(int sock){
	pthread_t in_t, out_t;
	void *res;
	int s;

	puts("チャットを開始します");
        s = pthread_create(&in_t, NULL, input, &sock);
        if(s != 0){
		fprintf(stderr, "pthread_create: err");
                exit(1);
        }
 
        s = pthread_create(&out_t, NULL, output, &sock);
        if(s != 0){
              	fprintf(stderr, "pthread_create2: err\n");
               	exit(1);
      	}
 
     	pthread_join(in_t, &res);
	close(sock);

	return 0;
}

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
			puts("ピアソケットがクローズされました");
			exit(1);
		}

		read(sock, in, sizeof in);
		printf("\n相手:%s私:", in);
	}
	exit(0);
}
