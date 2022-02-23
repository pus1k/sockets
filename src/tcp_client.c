#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MESSAGE_SIZE 10
#define PORT 8080

int check(const int result, const char* const err)
{
	if (result < 0)
		printf("CLIENT:\033[1m\033[31m %s\033[0m", err), perror(""), exit(EXIT_FAILURE);

	return result;
}
int main(int argc, char* argv[])
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
	if (argc == 2 && atoi(argv[1]) != 0)
	{
		char message[MESSAGE_SIZE];
		for (int i = 0, n = atoi(argv[1]); i < n; i++)
		{
			sleep(i + 1);
			sprintf(message, "%d", i + 1);
			check(send(client_socket, message, strlen(message), 0), "SEND");
		}
		check(send(client_socket, NULL, 0, 0), "SEND");
	}
	else
	{
		printf("\n\n./tcp_client [NUMBER OF MESSAGES]\n\n");
	}
	printf("CLIENT: SEE YA!\n");
	close(client_socket);
	return 0;
}