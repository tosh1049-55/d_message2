cli.out sever.out: cli.c sever.c comunicates.c
	gcc -pthread comunicates.c sever.c -o sever.out
	gcc -pthread comunicates.c cli.c -o cli.out

clean:
	rm -i *.out

cli_run:
	./cli.out localhost 100

sever_run:
	sudo ./sever.out 100
