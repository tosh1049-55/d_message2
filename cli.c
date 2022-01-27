#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int massege(int sock);
int output(int sock);
int input(int sock);
int open_fd(int fd, FILE **finp, FILE **fintp);
int conection(char *host, char *service);

int main(int argc, char *argv[]){
	int sock;
	FILE *in, *out;
	
	if(argc < 3){
		fprintf(stderr, "%s server port: please!!\n", argv[0]);
		exit(1);
	}
	printf("サーバー%sのポート%sに接続します\n", argv[1],argv[2]);
	sock = conection(argv[1], argv[2]);
	puts("接続に成功しました");
	massege(sock);
	exit(0);
}

int massege(int sock){
	int pi, sock2;
	
	sock2 = dup(sock);
	pi = fork();
	if(pi < 0){
		fputs("fork1 err\n", stderr);
		exit(1);
	}
	if(pi == 0) output(sock);
	pi = fork();
	if(pi < 0){
		fputs("fork2 err\n", stderr);
		exit(1);
	}
	if(pi == 0) input(sock2);

	for(;;){
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
		printf("\n相手:%s私:", in);
	}
	exit(0);
}

//finpがfdの読み込み専用のやつ。fintpは書き込み専用のやつ
int open_fd(int fd, FILE **finp, FILE **fintp){
	int fd2;
	FILE *fin1, *fin2;

	if((fd2 = dup(fd)) < 0){
		fputs("dup: err\n", stderr);
		return 1;
	}
	if((fin1 = fdopen(fd2, "r")) == NULL){
		fputs("fin1 open: err\n", stderr);
		return 1;
	}
	if((fin2 = fdopen(fd, "w")) == NULL){
		fputs("fin2 open: err\n", stderr);
		return 1;
	}
	printf("%d,%d\n",fd, fd2);
	*finp = fin1;
	*fintp = fin2;

	return 0;
}

//hostでipアドレスまたはドメインを指定 serviceでポート番号またはサービス名を指定
int conection(char *host, char *service){
	int sock, err;
	struct addrinfo hints, *res, *ai;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if((err = getaddrinfo(host, service, &hints, &res)) != 0){
		fprintf(stderr, "getaddrinfo\n");
		exit(1);
	}
	for(ai = res;ai;ai = ai->ai_next){
		sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if(sock < 0) continue;
		if(connect(sock, ai->ai_addr, ai->ai_addrlen) < 0){
			close(sock);
			continue;
		}
		freeaddrinfo(res);
		return sock;
	}
	fprintf(stderr, "socket(2)/connect(2) failed\n");
	freeaddrinfo(res);
	exit(1);
}
