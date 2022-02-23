#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MESSAGE_SIZE 10
#define IP_ADDRESS_SIZE 16
#define PORT 8080

void rip()
{
	while (waitpid(-1, NULL, WNOHANG) > 0) { }
}
int check(const int result, const char* const err)
{
	if (result < 0)
		printf("SERVER:\033[1m\033[31m %s\033[0m", err), perror(""), exit(EXIT_FAILURE);

	return result;
}
void work(int client_socket, struct sockaddr_in client_address)
{
	char message[MESSAGE_SIZE];
	memset(&message, 0, sizeof(message));

	char* ip_str = inet_ntoa(client_address.sin_addr);
	int client_port = ntohs(client_address.sin_port);

	while (recv(client_socket, message, MESSAGE_SIZE, 0) > 0)
	{
		printf("SERVER: (%s:%d) %s\n", ip_str, client_port, message);
	}
}
int main(void)
{
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	check(server_socket, "SOCKET");

	struct sockaddr_in server_address, client_address;
	memset(&server_address, 0, sizeof(server_address));
	memset(&client_address, 0, sizeof(client_address));

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(PORT);

	check(bind(server_socket, (const struct sockaddr*)&server_address, sizeof(server_address)), "BIND");
	listen(server_socket, 3);

	signal(SIGCHLD, rip);
	printf("SERVER:\033[1m\033[32m READY\033[0m\n");
	for (socklen_t addr_size = sizeof(client_address);;)
	{
		int client_socket = accept(server_socket, (struct sockaddr * restrict) & client_address, &addr_size);
		if (client_socket != -1)
		{
			printf("SERVER: NEW CONNECTION %d\n", ntohs(client_address.sin_port));
			if (check(fork(), "FORK") == 0)
			{
				close(server_socket);
				work(client_socket, client_address);

				printf("SERVER:  DISCONNECTION %d\n", ntohs(client_address.sin_port));

				close(client_socket);
				exit(EXIT_SUCCESS);
			}
			close(client_socket);
		}
	}
}