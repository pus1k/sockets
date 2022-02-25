CC=gcc
CFLAGS=-Wall -Werror -Wpedantic -Wextra

all: udp tcp pthread select

udp: bin/udp_server bin/udp_client

tcp: bin/tcp_server bin/tcp_client

pthread: bin/pthread_server bin/pthread_client

select: bin/select_server bin/select_client


bin/udp_server: build/udp_server.o
	$(CC) -o bin/udp_server build/udp_server.o

bin/udp_client: build/udp_client.o 
	$(CC) -o bin/udp_client build/udp_client.o

build/udp_client.o: src/lab1_udp/udp_client.c
	$(CC) $(CFLAGS) -c src/lab1_udp/udp_client.c -o build/udp_client.o

build/udp_server.o: src/lab1_udp/udp_server.c
	$(CC) $(CFLAGS) -c src/lab1_udp/udp_server.c -o build/udp_server.o


bin/tcp_server: build/tcp_server.o
	$(CC) -o bin/tcp_server build/tcp_server.o

bin/tcp_client: build/tcp_client.o 
	$(CC) -o bin/tcp_client build/tcp_client.o

build/tcp_client.o: src/lab2_tcp/tcp_client.c
	$(CC) $(CFLAGS) -c src/lab2_tcp/tcp_client.c -o build/tcp_client.o

build/tcp_server.o: src/lab2_tcp/tcp_server.c
	$(CC) $(CFLAGS) -c src/lab2_tcp/tcp_server.c -o build/tcp_server.o


bin/pthread_server: build/pthread_server.o
	$(CC) -o bin/pthread_server build/pthread_server.o -lpthread

bin/pthread_client: build/pthread_client.o 
	$(CC) -o bin/pthread_client build/pthread_client.o 

build/pthread_client.o: src/lab3_pthread/pthread_client.c
	$(CC) $(CFLAGS) -c src/lab3_pthread/pthread_client.c -o build/pthread_client.o

build/pthread_server.o: src/lab3_pthread/pthread_server.c
	$(CC) $(CFLAGS) -c src/lab3_pthread/pthread_server.c -o build/pthread_server.o -lpthread


bin/select_server: build/select_server.o
	$(CC) -o bin/select_server build/select_server.o

bin/select_client: build/select_client.o 
	$(CC) -o bin/select_client build/select_client.o

build/select_client.o: src/lab4_select/select_client.c
	$(CC) $(CFLAGS) -c src/lab4_select/select_client.c -o build/select_client.o

build/select_server.o: src/lab4_select/select_server.c
	$(CC) $(CFLAGS) -c src/lab4_select/select_server.c -o build/select_server.o


clean:
	rm -rf build/* bin/* data.txt

rebuild: clean all

.PHONY: clean