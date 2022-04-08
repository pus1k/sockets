#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdbool.h>

// SOLUT THE PROBLEM WITH SHARED MEMORY:
// PTHREAD OR MEM LOL KEK


#define MESSAGE_SIZE 256
#define PORT 8080

int size = 0, capacity = 50;

struct User {
    struct sockaddr_in m_address;
    char m_name[30];
} *user_list;

int check(int result, char* msg)
{
    if (result < 0) {
        char buff[500];
        sprintf(buff, "SERVER:\033[1m\033[31m%s\033[0m", msg), perror(buff), exit(EXIT_FAILURE);
    }
    printf("SERVER:\033[1m\033[32m%s\033[0m\n", msg);
    return result;
}

void rip() { while (waitpid(-1, NULL, WNOHANG) > 0); }

void add_user(struct sockaddr_in client_address, char* message)
{
	if (size == capacity) {
		capacity *= 2;
		user_list = realloc(user_list, sizeof(*user_list) * capacity);
	}
    message[strlen(message) - 1] = '\0';
	
    strcpy(user_list[size].m_name, message);
    memcpy(&user_list[size].m_address, &client_address, sizeof(client_address));
    if (ntohs(client_address.sin_port) == ntohs(user_list[size].m_address.sin_port))
        printf("SERVER:NEW USER %s %d\n", user_list[size].m_name, ntohs(user_list[size].m_address.sin_port)), size++;
}
void work(int socket, struct sockaddr_in client_address, char* message)
{
    int num = (message[0] - '0') * 10 + (message[1] - '0');
    if (num == 27) {
        add_user(client_address, message + 2);
    } else if (num == 54) {
        printf("%d\n", size);
        for (int i = 0; i < size; i++) {
            sendto(socket, message + 1, strlen(message), MSG_WAITALL, (struct sockaddr*)&user_list[i].m_address, sizeof(user_list[i].m_address));
        }
    } else if (num == 81) {
        // del user
    }
    exit(EXIT_SUCCESS);
}
int main(void)
{
	if ((user_list = malloc(sizeof(*user_list) * capacity)) == NULL) { exit(EXIT_FAILURE); }

    int server_socket = check(socket(AF_INET, SOCK_DGRAM, 0), "SOCKET");

    struct sockaddr_in server_address, client_address;
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);
    check(bind(server_socket, (const struct sockaddr*)&server_address, sizeof(server_address)), "BIND");

    signal(SIGCHLD, rip);

    while (true) {
        socklen_t addr_len = sizeof(client_address);
        char message[MESSAGE_SIZE];

        memset(&message, 0, MESSAGE_SIZE);
        memset(&client_address, 0, sizeof(client_address));

        recvfrom(server_socket, message, MESSAGE_SIZE, 0, (struct sockaddr*)&client_address, &addr_len);

        if (fork() == 0) { work(server_socket, client_address, message); }
    }
    return 0;
}