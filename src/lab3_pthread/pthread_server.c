#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MESSAGE_SIZE 10
#define PORT 8080

#define YELLOW 33
#define GREEN 32
#define RED 31

void color_print(int color, char* str) { printf("SERVER:\033[1m\033[%dm%s\033[0m", color, str); }

int check(int result, char* err)
{
	if (result < 0) { color_print(RED, err), perror(""), exit(EXIT_FAILURE); }
	return result;
}

typedef struct
{
	int client_socket, client_port;
} arguments;

void* work(void* raw_args)
{
	arguments args;
	args.client_port = ((arguments*)raw_args)->client_port;
	args.client_socket = ((arguments*)raw_args)->client_socket;

	int fd = check(open("data.txt", O_CREAT | O_WRONLY | O_APPEND, S_IRWXU), " OPEN");

	color_print(YELLOW, " CONNECT ");
	printf("%d\n", args.client_port);

	char message[MESSAGE_SIZE], buff[MESSAGE_SIZE * 4];
	memset(&message, 0, sizeof(message));
	memset(&buff, 0, sizeof(buff));

	pthread_mutex_t my_lock;
	while (recv(args.client_socket, message, MESSAGE_SIZE, 0) > 0)
	{
		sprintf(buff, "SERVER: [\033[1m\033[34m%d\033[0m] %s\n", args.client_port, message);
		pthread_mutex_lock(&my_lock);

		write(1, buff, strlen(buff));
		write(fd, buff, strlen(buff));

		pthread_mutex_unlock(&my_lock);
	}
	close(fd);
	close(args.client_socket);

	color_print(YELLOW, " DISCONNECT ");
	printf("%d\n", args.client_port);

	pthread_exit(NULL);
}
int main(void)
{
	int client_socket, server_socket = socket(AF_INET, SOCK_STREAM, 0);
	check(server_socket, " SOCKET");

	struct sockaddr_in server_address, client_address;
	memset(&server_address, 0, sizeof(server_address));
	memset(&client_address, 0, sizeof(client_address));

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(PORT);

	check(bind(server_socket, (const struct sockaddr*)&server_address, sizeof(server_address)), " BIND");

	pthread_t thr;
	listen(server_socket, 3);

	color_print(GREEN, " READY\n");
	for (socklen_t addr_size = sizeof(client_address);;)
	{
		client_socket = accept(server_socket, (struct sockaddr * restrict)(&client_address), &addr_size);
		if (client_socket != -1)
		{
			arguments args = { client_socket, ntohs(client_address.sin_port) };
			pthread_create(&thr, NULL, work, (void*)&args);
		}
	}
}