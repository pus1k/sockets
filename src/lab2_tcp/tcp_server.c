#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
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

void rip()
{
	while (waitpid(-1, NULL, WNOHANG) > 0) { }
}

void work(int client_socket, struct sockaddr_in client_address)
{
	char message[MESSAGE_SIZE];
	memset(&message, 0, sizeof(message));

	int client_port = ntohs(client_address.sin_port);

	while (recv(client_socket, message, MESSAGE_SIZE, 0) > 0)
	{
		printf("SERVER: [\033[1m\033[34m%d\033[0m] %s\n", client_port, message);
	}
}
int main(void)
{
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	check(server_socket, " SOCKET");

	struct sockaddr_in server_address, client_address;
	memset(&server_address, 0, sizeof(server_address));
	memset(&client_address, 0, sizeof(client_address));

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(PORT);

	check(bind(server_socket, (const struct sockaddr*)&server_address, sizeof(server_address)), " BIND");
	listen(server_socket, 3);

	signal(SIGCHLD, rip);
	color_print(GREEN, " READY\n");
	for (socklen_t addr_size = sizeof(client_address);;)
	{
		int client_socket = accept(server_socket, (struct sockaddr * restrict) & client_address, &addr_size);
		if (client_socket != -1)
		{
			color_print(YELLOW, " CONNECT ");
			printf("%d\n", ntohs(client_address.sin_port));
			if (check(fork(), " FORK") == 0)
			{
				close(server_socket);
				work(client_socket, client_address);

				color_print(YELLOW, " DISCONNECT ");
				printf("%d\n", ntohs(client_address.sin_port));

				close(client_socket);
				exit(EXIT_SUCCESS);
			}
			close(client_socket);
		}
	}
}