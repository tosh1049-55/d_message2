#define _GNU_SOURCE
#define MESSAGE_MAX_LINE 4
#define MAX_LINE 1000
#define CLI 1
#define SERVER 0

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
#include <signal.h>

void *input(void *arg);
void *output(void *arg);
int message_put(int *poses, char *name, char *message);
int put_control_init();
int put_control_end();
int put_control_put(char *name, char *str);
int put_control_up();
int put_control_down();
long count_line(char *str);
int check_line(char *src);
void sigint_hand(int sig);
int line_check(FILE *fd);

//送信前メッセージ出力の先頭
static int pos[2] = {30, 0};
static int pos_front[2] = {0,0};
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_control = PTHREAD_MUTEX_INITIALIZER;

static struct control_dates{
	int pos_tail;
	FILE *out_fd;
	FILE *in_fd;
	int file_front;
	long line_long[MAX_LINE];	//fileの行ごとの文字数(\nも含む)
} control_date;

void sigint_hand(int sig){
	put_control_end();

	exit(0);
}

int message(int sock, int who){
	pthread_t in_t, out_t;
	struct sigaction act;
	void *res;
	int s;
	int x, y;

	act.sa_handler = sigint_hand;
	puts("チャットを開始します");
	sigaction(SIGINT, &act, NULL);
	put_control_init(who);
	getmaxyx(stdscr, y, x);
	y -= (MESSAGE_MAX_LINE + 1);
	pos[0] = y;
	move(y,0);
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
	put_control_end();

	return 0;
}

void *output(void *arg){
	int sock = *(int *)(arg);
	char in[1024];
	
	for(;;){
		int i, in_char;
		int x, y;

		getmaxyx(stdscr, y, x);
		y -= (MESSAGE_MAX_LINE + 1);

		fputs("私:", stdout);
		for(i = 0;;i++){
			in_char = getch();	
			if(in_char == '.')
				break;
			pthread_mutex_lock(&mtx);
			if(in_char == '\n'){
				pos[0]++;pos[1] = 0;
				in[i] = (char)in_char;
				move(pos[0], pos[1]);
			}else if(in_char == KEY_BACKSPACE){
				if(i == 0){
					beep();
					i--;
				}else{
					move(pos[0], pos[1]-1);
					delch();
					i -= 2;
					pos[1]--;
				}
			}else if(in_char == KEY_UP){
				put_control_up();	
				i--;
				move(pos[0], pos[1]);
			}else if(in_char == KEY_DOWN){
				put_control_down();
				i--;
				move(pos[0], pos[1]);
			}else{
				pos[1]++;
				in[i] = (char)in_char;
			}
			move(0,30);
			printw("%d", in[i]);
			move(pos[0], pos[1]);
			pthread_mutex_unlock(&mtx);
		}
		in[i+1] = '\0';
		write(sock, in, sizeof in);

		put_control_put("you", in);

		for(i = 0;i<100;i++){
			int r;
			for(r=y;r<(y + MESSAGE_MAX_LINE + 1);r++)
				mvaddch(r, i, ' ');
		}

		getmaxyx(stdscr, y, x);
		y -= (MESSAGE_MAX_LINE + 1);
		
		move(y,0);
		pthread_mutex_lock(&mtx);
		pos[0] = y;pos[1] = 0;
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

		put_control_put("guest", in);

		pthread_mutex_lock(&mtx);
		move(pos[0],pos[1]);
		pthread_mutex_unlock(&mtx);
	}
	exit(0);
}

//失敗したら-1を返す.who == 0でsever who == 1でcli
int put_control_init(int who){
	FILE *in_fd, *out_fd;
	
	initscr();
	keypad(stdscr, TRUE);
	if(who == 0){
		out_fd = fopen("message_sever", "a");
		in_fd = fopen("message_sever", "r");
	}else{
		out_fd = fopen("message_cli", "a");
		in_fd = fopen("message_cli", "r");
	}
		
	if(in_fd < 0){
		fputs("in_fd open err\n", stderr);
		return -1;
	}
	if(out_fd < 0){
		fputs("out_fd open err\n", stderr);
		return -1;
	}
	pthread_mutex_lock(&mtx_control);
	control_date.in_fd = in_fd;
	control_date.out_fd = out_fd;
	fseek(control_date.in_fd, 0, SEEK_SET);
	fseek(control_date.out_fd, -1, SEEK_END);
	control_date.pos_tail = line_check(in_fd);
	fseek(control_date.in_fd, 0, SEEK_SET);
	control_date.file_front = 0;
	pthread_mutex_unlock(&mtx_control);

	return 0;
}

//渡されたファイルディスクリプタを開放する
int put_control_end(){
	int i;

	pthread_mutex_lock(&mtx_control);
	fclose(control_date.in_fd);
	fclose(control_date.out_fd);
	pthread_mutex_unlock(&mtx_control);

	endwin();

	unlink("message_cli");
	unlink("message_sever");

	for(i = 0;i<10;i++)
		printf("%ld\n", control_date.line_long[i]);

	return 0;
}

//渡された文字列を端末に出力し、ファイルに保存する
int put_control_put(char *name, char *str){
	int pos_tail, line_tail, l, line_num;
	char buf[4096], *i;
	pthread_mutex_lock(&mtx_control);
	move(control_date.pos_tail, 0);
	printw("%s:\n%s\n", name, str);
	line_num = check_line(str) + 2;
	line_tail = control_date.pos_tail;
	pos_tail = (line_tail + line_num);
	control_date.pos_tail = pos_tail;

	//行ごとの文字数を詰める
	strcpy(buf, name);
	strcat(buf, ":\n");
	strcat(buf, str);
	strcat(buf, "\n\n");

	fseek(control_date.out_fd, -1, SEEK_END);
	fputs(buf, control_date.out_fd);
	i = buf;
	l = 0;
	while(l < line_num){
		char *p;
		p = i;
		if(!(i = strchr(p, '\n'))){
			control_date.line_long[l + line_tail + control_date.file_front] = count_line(p);
			break;
		}
		*i++ == '\0';
		control_date.line_long[l + line_tail + control_date.file_front] = count_line(p);
		l++;
	}	

	pthread_mutex_unlock(&mtx_control);
	
	return pos_tail;
}

//渡されたファイルを現在の表示から上へスクロールする
//上にできなかったら-1を返す
int put_control_up(){
	int i, line_long_num;
	long seek_num;
	char buf[4096];

	pthread_mutex_lock(&mtx_control);
	//今いる行の行番号を計算する
	line_long_num = control_date.file_front;
	//移動する行の先頭の文字の絶対アドレスを計算
	seek_num = ftell(control_date.in_fd) + control_date.line_long[line_long_num];
	if((line_long_num + control_date.pos_tail) >= MAX_LINE){
		beep();
		return -1;
	}
	fseek(control_date.in_fd, seek_num, SEEK_SET);
	move(0,0);
	for(i=0;i<(control_date.pos_tail+1);i++){
		fgets(buf, sizeof buf, control_date.in_fd);
		printw("%s", buf);
	}
	control_date.file_front++;
	control_date.pos_tail--;
	fseek(control_date.in_fd, seek_num, SEEK_SET);
	fseek(control_date.out_fd, -1, SEEK_END);

	move (0, 20);
	printw("%d", seek_num);
	pthread_mutex_unlock(&mtx_control);

	return 0;
}

//渡されたファイルを現在の表示から下へスクロールする
int put_control_down(){
	int i, line_long_num;
	long seek_num;
	char buf[4096];

	pthread_mutex_lock(&mtx_control);
	//一つ前の行の行番号を取得
	line_long_num = control_date.file_front - 1;
	seek_num = ftell(control_date.in_fd) - control_date.line_long[line_long_num];
	if(line_long_num  < 0){
		beep();

		return -1;
	}
	fseek(control_date.in_fd, seek_num, SEEK_SET);
	move(0,0);
	for(i=0;i < (control_date.pos_tail+1);i++){
		fgets(buf, sizeof buf, control_date.in_fd);
		printw("%s", buf);
	}
	control_date.file_front--;
	control_date.pos_tail++;
	fseek(control_date.in_fd, seek_num, SEEK_SET);

	move (0, 20);
	printw("%d", seek_num);
	pthread_mutex_unlock(&mtx_control);

	return 0;
}

//渡された文字列を１つ目の改行または、\0が出るまで文字数をカウントする。なお、文字数には改行を含める
long count_line(char *str){
	int count;

	for(count = 0;;count++){
		if(str[count] == '\n' || str[count] == '\0') break;
	}

	return (count+1);
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

int line_check(FILE *fd){
	char buf;
	int line_num = 0;

	do{
		buf = getc(fd);
		if(buf == '\n')
			line_num++;
		if(line_num >= MAX_LINE) break;
	}while(buf != EOF && buf != '\0');

	return line_num;
}
