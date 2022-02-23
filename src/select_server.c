#include <arpa/inet.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MESSAGE_SIZE 10
#define MAX_USERS 3
#define PORT 8080

int check(const int result, const char* const err) { if (result < 0) { perror(err), exit(EXIT_FAILURE); } return result; }

int main(void)
{
	int client_socket, server_socket = socket(AF_INET, SOCK_STREAM, 0);
	check(server_socket, "SOCKET");

	struct sockaddr_in server_address, client_address, arr_address[MAX_USERS];
	memset(&server_address, 0, sizeof(server_address));
	memset(&client_address, 0, sizeof(client_address));

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(PORT);

	check(bind(server_socket, (const struct sockaddr*)&server_address, sizeof(server_address)), "BIND");
	listen(server_socket, MAX_USERS);

	fd_set current, temp;
	FD_ZERO(&current);
	FD_ZERO(&temp);
	FD_SET(server_socket, &current);
	FD_SET(server_socket, &temp);

	char message[MESSAGE_SIZE];
	socklen_t addr_size = sizeof(client_address);
	printf("SERVER: READY\n");
	for (;;)
	{
		check(select(FD_SETSIZE, &current, NULL, NULL, NULL), "SELECT");
		client_socket = check(accept(server_socket, (struct sockaddr*)(&client_address), &addr_size), "ACCEPT");
		memcpy(&arr_address[client_socket], &client_address, sizeof(client_address));
		FD_SET(client_socket, &temp);
		for (int fd = 0; fd < FD_SETSIZE; fd++)
		{
			if (FD_ISSET(fd, &current))
			{
				if (check(recv(fd, message, MESSAGE_SIZE, 0), "RECEIVE") == 0)
				{
					close(fd);
					FD_CLR(fd, &temp);
					memset(&arr_address[fd], 0, sizeof(arr_address[fd]));
				}
				else
				{
					printf("SERVER: (%d) %s\n", ntohs(arr_address[fd].sin_port), message);
				}
			}
		}
		memcpy(&current, &temp, sizeof(current));
	}
}