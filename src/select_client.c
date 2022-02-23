#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080

int check(const int result, const char* const err)
{
	if (result < 0)
		printf("CLIENT:\033[1m\033[31m %s\033[0m", err), perror(""), exit(EXIT_FAILURE);

	return result;
}

int main(int argc, char* argv[])
{
	int n = atoi(argv[1]);
	if (argc == 2 && n != 0)
	{
		int client_socket = socket(AF_INET, SOCK_STREAM, 0);
		check(client_socket, "SOCKET");

		struct sockaddr_in server_address;
		memset(&server_address, 0, sizeof(server_address));

		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = INADDR_ANY;
		server_address.sin_port = htons(PORT);

		check(connect(client_socket, (const struct sockaddr*)&server_address, sizeof(server_address)), "CONNECT");
		printf("CLIENT:\033[1m\033[32m READY\033[0m\n");
		for (;;)
		{
			sleep(n);
			check(send(client_socket, argv[1], strlen(argv[1]), 0), "SEND");
			printf("CLIENT:SEND\033[1m\033[32m SUCCESS\033[0m\n");
		}
	}
	else
	{
		printf("\n\n./bin/select_client [NUMBER]\n");
		printf("\t\tNUMBER --- value for sending and delaying each send.\n\n");
	}
	return 0;
}
