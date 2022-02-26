#include <arpa/inet.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define GREEN 32
#define RED 31

void color_print(int color, char* str) { printf("CLIENT:\033[1m\033[%dm%s\033[0m", color, str); }

int check(int result, char* err)
{
	if (result < 0) { color_print(RED, err), perror(""), exit(EXIT_FAILURE); }
	return result;
}

#define PORT 8080

int main(int argc, char* argv[])
{
	int n = atoi(argv[1]);
	if (argc == 2 && n != 0)
	{
		int client_socket = socket(AF_INET, SOCK_STREAM, 0);
		check(client_socket, " SOCKET");

		struct sockaddr_in server_address, client_address;
		memset(&server_address, 0, sizeof(server_address));
		memset(&client_address, 0, sizeof(client_address));

		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = INADDR_ANY;
		server_address.sin_port = htons(PORT);

		check(connect(client_socket, (const struct sockaddr*)&server_address, sizeof(server_address)), " CONNECT");

		socklen_t addr_size = sizeof(client_address);
		getsockname(client_socket, (struct sockaddr*)&client_address, &addr_size);

		color_print(GREEN, " READY\n");
		printf("CLIENT: MY PORT [\033[1m\033[34m%d\033[0m]\n", ntohs(client_address.sin_port));
		for (;;)
		{
			sleep(n);
			check(send(client_socket, argv[1], strlen(argv[1]), 0), " SEND");
		}
	}
	else
	{
		printf("\n\n./bin/select_client [NUMBER]\n");
		printf("\t\tNUMBER --- value for sending and delaying each send.\n\n");
	}
	return 0;
}
