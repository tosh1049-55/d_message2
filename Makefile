cli.out sever.out: cli.c sever.c
	gcc sever.c -o sever.out
	gcc cli.c -o cli.out

clean:
	rm -i *.out

cli_run:
	./cli.out

sever_run:
	./sever_run
