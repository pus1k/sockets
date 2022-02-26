#include <arpa/inet.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MESSAGE_SIZE 10
#define MAX_USERS 10
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

int main(void)
{
	int client_socket, server_socket = socket(AF_INET, SOCK_STREAM, 0);
	check(server_socket, " SOCKET");

	struct sockaddr_in server_address, client_address, arr_address[MAX_USERS];
	memset(&server_address, 0, sizeof(server_address));
	memset(&client_address, 0, sizeof(client_address));
	socklen_t addr_size = sizeof(client_address);

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(PORT);

	check(bind(server_socket, (const struct sockaddr*)&server_address, sizeof(server_address)), " BIND");
	listen(server_socket, MAX_USERS);

	fd_set talker, fdlist;
	FD_ZERO(&fdlist);
	FD_SET(server_socket, &fdlist);

	color_print(GREEN, " READY\n");
	for (char message[MESSAGE_SIZE];;)
	{
		memcpy(&talker, &fdlist, sizeof(fdlist));
		check(select(FD_SETSIZE, &talker, NULL, NULL, NULL), " SELECT");
		if (FD_ISSET(server_socket, &talker))
		{
			client_socket = check(accept(server_socket, (struct sockaddr*)(&client_address), &addr_size), " ACCEPT");
			memcpy(&arr_address[client_socket], &client_address, sizeof(client_address));
			FD_SET(client_socket, &fdlist);
			color_print(YELLOW, " CONNECT "), printf("%d\n", ntohs(client_address.sin_port));
		}

		for (int fd = 0; fd < FD_SETSIZE; fd++)
		{
			if (FD_ISSET(fd, &talker) && fd != server_socket)
			{
				if (check(recv(fd, message, MESSAGE_SIZE, 0), " RECEIVE") == 0)
				{
					FD_CLR(fd, &fdlist);
					color_print(YELLOW, " DISCONNECT "), printf("%d\n", ntohs(arr_address[fd].sin_port));
					memset(&arr_address[fd], 0, sizeof(arr_address[0]));
					close(fd);
				}
				else
				{
					printf("SERVER: [\033[1m\033[34m%d\033[0m] %s\n", ntohs(arr_address[fd].sin_port), message);
				}
			}
		}
	}
}