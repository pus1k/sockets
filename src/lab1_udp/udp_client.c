#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MESSAGE_SIZE 81
#define PORT 8080

#define YELLOW 33
#define GREEN 32
#define RED 31

void color_print(int color, char* str) { printf("CLIENT:\033[1m\033[%dm%s\033[0m", color, str); }

int check(int result, char* err)
{
	if (result < 0) { color_print(RED, err), perror(""), exit(EXIT_FAILURE); }
	return result;
}

void work(int client_socket, char* message, struct sockaddr_in server_address)
{
	sendto(client_socket, message, MESSAGE_SIZE, MSG_WAITALL, (const struct sockaddr*)&server_address, sizeof(server_address));
	recvfrom(client_socket, message, MESSAGE_SIZE, 0, (struct sockaddr * restrict) & server_address, (socklen_t * restrict)sizeof(server_address));
	color_print(33, " MESSAGE FROM SERVER "), printf("%s\n", message);
}
int main(int argc, char* argv[])
{
	int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
	check(client_socket, " SOCKET");

	struct sockaddr_in server_address, client_address;
	memset(&server_address, 0, sizeof(server_address));
	memset(&client_address, 0, sizeof(client_address));

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(PORT);

	check(bind(client_socket, (const struct sockaddr*)&client_address, sizeof(client_address)), " BIND");

	socklen_t addr_size = sizeof(client_address);
	getsockname(client_socket, (struct sockaddr*)&client_address, &addr_size);

	char message[MESSAGE_SIZE];
	color_print(GREEN, " READY\n");
	printf("CLIENT: MY PORT [\033[1m\033[34m%d\033[0m]\n", ntohs(client_address.sin_port));
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
		while (!strcmp(message, "EXIT"))
		{
			printf("> ");
			fgets(message, MESSAGE_SIZE, stdin);
			message[strlen(message) - 1] = '\0';
			work(client_socket, message, server_address);
		}
	}
	color_print(GREEN, " BYE SWEETIE!\n");
	close(client_socket);
	return 0;
}