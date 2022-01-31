cli.out sever.out: cli.c sever.c comunicates.c
	gcc comunicates.c sever.c -o sever.out
	gcc comunicates.c cli.c -o cli.out

clean:
	rm -i *.out

cli_run:
	./cli.out localhost 100

sever_run:
	sudo ./sever.out 100
