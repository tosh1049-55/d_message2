#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>

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
		int sock2;

		sock = accept(server, &addr, &addr_len);
		sock2 = dup(sock);

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

	for(;;){
		read(sock, in, sizeof in);
		fputs("\n相手:%s", in);
		fputs("私:", stdout);
	}
	exit(0);
}

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
