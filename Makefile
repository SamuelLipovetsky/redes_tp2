all:
	gcc -Wall -c common.c
	gcc -Wall client.c common.o -o client
	gcc -Wall server-mt.c common.o -o server
	
clean:
	rm common.o client server 