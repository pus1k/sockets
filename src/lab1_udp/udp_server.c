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

void color_print(int color, char* str) { printf("SERVER:\033[1m\033[%dm%s\033[0m", color, str); }

int check(int result, char* err)
{
	if (result < 0) { color_print(RED, err), perror(""), exit(EXIT_FAILURE); }
	return result;
}

int main(void)
{
	int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
	check(server_socket, " SOCKET");

	struct sockaddr_in server_address, client_address;
	memset(&server_address, 0, sizeof(server_address));
	memset(&client_address, 0, sizeof(client_address));

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(PORT);
	check(bind(server_socket, (const struct sockaddr*)&server_address, sizeof(server_address)), " BIND");

	color_print(GREEN, " READY\n");
	for (char message[MESSAGE_SIZE];;)
	{
		int length = sizeof(client_address);
		memset(&message, 0, sizeof(message));
		recvfrom(server_socket, message, MESSAGE_SIZE, 0, (struct sockaddr*)&client_address, (socklen_t*)&length);

		printf("SERVER: [\033[1m\033[34m%d\033[0m] %s\n", ntohs(client_address.sin_port), message);

		strcat(message, "_SERVER");
		sendto(server_socket, message, MESSAGE_SIZE, MSG_WAITALL, (const struct sockaddr*)&client_address, sizeof(client_address));
	}
}