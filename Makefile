all:
	gcc -g -Wall -o cshell cshell.c
	
clean:
	-rm cshell
