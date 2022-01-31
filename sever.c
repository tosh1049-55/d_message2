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

int output(int sock);
int input(int sock);
int listen_socket(char *port);

int main(int argc, char *argv[]){
	int server, sock;
	struct sockaddr addr;
	FILE *fd;
	socklen_t addr_len;
	char buf[1024], in[2048];
	pid_t pi;

	if(argc < 2){
		fprintf(stderr, "%s port\n", argv[0]);
		exit(1);
	}
	printf("ポート%sを使うサーバーを立てます\n",argv[1]);
	server = listen_socket(argv[1]);
	for(;;){
		int nfds, ready;
		fd_set readfds, writefds;
		struct timeval timeout;
		
		sock = accept(server, &addr, &addr_len);

		//selectする
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_SET(sock, &readfds);
		FD_SET(sock, &writefds);

		timeout.tv_sec = 10;
		timeout.tv_usec = 0;
		nfds = 1;

		puts("sockのチェック中です");
		ready = select(nfds, &readfds, &writefds, NULL, &timeout);
		if(ready == -1){
			fputs("select err\n", stderr);
			exit(1);
		}else if(ready == 0)
			fputs("timeout\n", stderr);

		if(sock < 0) {
			fprintf(stderr, "accept failed\n");
			exit(0);
		}
		fd = fdopen(sock, "r+");
		if(fd < 0){
			fputs("fdopen: err\n", stderr);
			exit(1);
		}
		puts("チャットを開始します");
		pi = fork();
		if(pi < 0){
			fprintf(stderr, "pi: err");
			exit(1);
		}
		if(pi == 0) output(sock);
		pi = fork();
		if(pi < 0){
			fprintf(stderr, "fork2: err\n");
			exit(1);
		}
		if(pi == 0) input(sock);
	}
	close(sock);

	return 0;
}
	
//int output(int sock){
//	char in[1024];
//	
//	for(;;){
//		fputs("私:", stdout);
//		fgets(in, sizeof in, stdin);
//		write(sock, in, sizeof in);
//		puts("送信しました");
//	}
//
//	exit(0);
//}

//int input(int sock){
//	char in[1024];
//
//	int nfds, ready;
 //	fd_set readfds;
//	struct timeval timeout;
//
//	//selectする
//	FD_ZERO(&readfds);
//	FD_SET(sock, &readfds);
//
//	timeout.tv_sec = 10;
//	timeout.tv_usec = 0;
//	nfds = 1;
//
//	puts("sockのチェック中です");
 //	ready = select(nfds, &readfds, NULL, NULL, &timeout);
//	if(ready == -1){
//		fputs("select err\n", stderr);
//		exit(1);
//	}else if(ready == 0)
//		fputs("timeout\n", stderr);
//
//	for(;;){
//		read(sock, in, sizeof in);
//		fputs("\n相手:%s", in);
//		fputs("私:", stdout);
//	}
//	exit(0);
//}//

//hostでipアドレスまたはドメインを指定 serviceでポート番号またはサービス名を指定
int listen_socket(char *port){
	int sock, err;
	struct addrinfo hints, *res, *ai;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
	if((err = getaddrinfo(NULL, port, &hints, &res)) != 0){
		fprintf(stderr, "getaddrinfo\n");
		exit(1);
	}
	for(ai = res;ai;ai = ai->ai_next){
		sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if(sock < 0) {
			printf("socketの問題です\n");
			continue;
		}
		if(bind(sock, ai->ai_addr, ai->ai_addrlen) < 0){
			close(sock);
			printf("%s,%d\n", ai->ai_addr->sa_data, ai->ai_addrlen);
			printf("bindの問題です\n");
			continue;
		}
		if(listen(sock, 3) < 0){
			close(sock);
			printf("listenの問題です\n");
			continue;
		}
		freeaddrinfo(res);
		return sock;
	}
	fprintf(stderr, "failed to listen socket\n");
	freeaddrinfo(res);
	exit(1);
}
