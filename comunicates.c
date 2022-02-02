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
int message_put(int *poses, char *name, char *message);
int check_line(char *src);

//送信前メッセージ出力の先頭
static int pos[2] = {30, 0};
static int pos_front[2] = {0,0};
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_front = PTHREAD_MUTEX_INITIALIZER;

int message(int sock){
	pthread_t in_t, out_t;
	void *res;
	int s;

	puts("チャットを開始します");
	initscr();
	move(30,0);
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
	endwin();

	return 0;
}

void *output(void *arg){
	int sock = *(int *)(arg);
	char in[1024];
	
	for(;;){
		int i;
		fputs("私:", stdout);
		for(i = 0;;i++){
			in[i] = getch();	
			if(in[i] == '.')
				break;
			pthread_mutex_lock(&mtx);
			if(in[i] == '\n'){
				pos[0]++;pos[1] = 0;
				move(pos[0], pos[1]);
			}else if(in[i] == '\x7f'){
				delch();
				move(pos[0], pos[1]);
				delch();
				if(i == 0)
					i--;
				else{
					move(pos[0], pos[1]-1);
					delch();
					i -= 2;
					pos[1] -= 1;
				}
			}else
				pos[1]++;
			pthread_mutex_unlock(&mtx);
		}
		in[i+1] = '\0';
		write(sock, in, sizeof in);

		pthread_mutex_lock(&mtx_front);
		message_put(pos_front, "you", in);
		move(30,0);
		pthread_mutex_unlock(&mtx_front);

		for(i = 0;i<100;i++){
			int r;
			for(r=30;r<40;r++)
				mvaddch(r, i, ' ');
		}
		move(30,0);
		pthread_mutex_lock(&mtx);
		pos[0] = 30;pos[1] = 0;
		pthread_mutex_unlock(&mtx);
	}

	exit(0);
}

void *input(void *arg){
	int sock = *(int *)arg, nfds, point;
	char in[1024];
	struct pollfd fds[1];

	for(;;){
		fds[0].fd = sock;
		fds[0].events = POLLRDHUP;
		nfds = poll(fds, 1, 0);
		if(nfds == -1){
			fputs("poll: err\n", stderr);
			return NULL;
		}
		if(fds[0].revents & POLLRDHUP){
			puts("ピアソケットがクローズされました");
			return NULL;
		}

		read(sock, in, sizeof in);

		pthread_mutex_lock(&mtx_front);
		message_put(pos_front, "guest", in);
		pthread_mutex_unlock(&mtx_front);

		pthread_mutex_lock(&mtx);
		move(pos[0],pos[1]);
		pthread_mutex_unlock(&mtx);
	}
	exit(0);
}

//メッセージの先頭位置　メッセージの名前　メッセージ本文
int message_put(int *poses, char *name, char *message){
	move(poses[0], poses[1]);
	printw("%s:%s", name, message);
	poses[0] += (check_line(message) + 1);

	return 0;
}

//\0まで調べる
int check_line(char *src){
	int i, count = 0;

	for(i=0;;i++){
		if(src[i] == '\n') count++;
		if(src[i] == '\0') break;
	}

	return (count + 1);
}
