#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <pthread.h>

#define MESSAGE_SIZE 256
#define PORT 8080

static pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;


struct User {
    struct sockaddr_in m_address;
    char m_name[30];
};

struct ServerData {
    size_t size, capacity;
    struct User *user_list;
} *data;

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
    pthread_mutex_lock(&user_mutex);
	
    if (data->size == data->capacity) {
		data->capacity *= 2;
		data->user_list = realloc(data->user_list, sizeof(*data->user_list) * data->capacity);
	}
    message[strlen(message) - 1] = '\0';
	
    strcpy(data->user_list[data->size].m_name, message);
    memcpy(&data->user_list[data->size].m_address, &client_address, sizeof(client_address)), data->size++;
    
    pthread_mutex_unlock(&user_mutex);
    
    if (ntohs(client_address.sin_port) == ntohs(data->user_list[data->size - 1].m_address.sin_port))
        printf("SERVER:NEW USER %s %d\n", data->user_list[data->size - 1].m_name, ntohs(data->user_list[data->size - 1].m_address.sin_port));
}
void work(int socket, struct sockaddr_in client_address, char* message)
{
    int num = (message[0] - '0') * 10 + (message[1] - '0');
    if (num == 27) {
        add_user(client_address, message + 2);
    } else if (num == 54) {
        printf("%ld\n", data->size);
        for (size_t i = 0; i < data->size; i++) {
            sendto(socket, message + 1, strlen(message), MSG_WAITALL, (struct sockaddr*)&data->user_list[i].m_address, sizeof(data->user_list[i].m_address));
        }
    } else if (num == 81) {
        // del user
    }
    exit(EXIT_SUCCESS);
}
void* create_shared_memory(size_t size) {
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}
int main(void)
{
	if ((data = create_shared_memory(sizeof(*data))) == NULL) { exit(EXIT_FAILURE); }
    data->size = 0, data->capacity = 50;
    if ((data->user_list = create_shared_memory(sizeof(*data->user_list) * data->capacity)) == NULL) { exit(EXIT_FAILURE); }

    int server_socket = check(socket(AF_INET, SOCK_DGRAM, 0), "SOCKET");

    struct sockaddr_in server_address, client_address;
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);
    check(bind(server_socket, (const struct sockaddr*)&server_address, sizeof(server_address)), "BIND");

    signal(SIGCHLD, rip);

    for(char message[MESSAGE_SIZE]; true; memset(&message, 0, MESSAGE_SIZE), memset(&client_address, 0, sizeof(client_address))) {
        socklen_t addr_len = sizeof(client_address);
        recvfrom(server_socket, message, MESSAGE_SIZE, 0, (struct sockaddr*)&client_address, &addr_len);
        if (fork() == 0) { work(server_socket, client_address, message); }
    }
    return 0;
}