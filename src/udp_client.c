#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MESSAGE_SIZE 81
#define PORT 8080

int check(const int result, const char* const err)
{
	if (result < 0)
		printf("CLIENT:\033[1m\033[31m %s\033[0m", err), perror(""), exit(EXIT_FAILURE);

	return result;
}
void work(int client_socket, char* message, struct sockaddr_in server_address)
{
	sendto(client_socket, message, MESSAGE_SIZE, MSG_WAITALL, (const struct sockaddr*)&server_address, sizeof(server_address));
	recvfrom(client_socket, message, MESSAGE_SIZE, 0, (struct sockaddr * restrict) & server_address, (socklen_t * restrict)sizeof(server_address));
	printf("CLIENT: MESSAGE FROM SERVER: %s\n", message);
}
int main(int argc, char* argv[])
{
	int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
	check(client_socket, "SOCKET");

	struct sockaddr_in server_address, client_address;
	memset(&server_address, 0, sizeof(server_address));
	memset(&client_address, 0, sizeof(client_address));

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(PORT);

	check(bind(client_socket, (const struct sockaddr*)&client_address, sizeof(client_address)), "BIND");

	char message[MESSAGE_SIZE];
	printf("CLIENT:\033[1m\033[32m READY\033[0m\n");
	if (argc == 2 && atoi(argv[1]) != 0)
	{
		for (int i = 0, n = atoi(argv[1]); i < n; i++)
		{
			sleep(i + 1);
			sprintf(message, "%d", i + 1);
			work(client_socket, message, server_address);
		}
	}
	else
	{
		for (; !strcmp(message, "EXIT");)
		{
			printf("> ");
			fgets(message, MESSAGE_SIZE, stdin);
			message[strlen(message) - 1] = '\0';
			work(client_socket, message, server_address);
		}
	}
	close(client_socket);
	printf("CLIENT: SEE YA!\n");
	return 0;
}