#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>

int output(int sock){
	char in[1024];
	
	for(;;){
		fputs("私:", stdout);
		fgets(in, sizeof in, stdin);
		write(sock, in, sizeof in);
		puts("送信しました");
	}

	exit(0);
}

int input(int sock){
	char in[1024];

	int nfds, ready;
 	fd_set readfds;
	struct timeval timeout;

	//selectする
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);

	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	nfds = 1;

	puts("sockのチェック中です");
 	ready = select(nfds, &readfds, NULL, NULL, &timeout);
	if(ready == -1){
		fputs("select err\n", stderr);
		exit(1);
	}else if(ready == 0)
		fputs("timeout\n", stderr);

	for(;;){
		read(sock, in, sizeof in);
		fputs("\n相手:%s", in);
		fputs("私:", stdout);
	}
	exit(0);
}
