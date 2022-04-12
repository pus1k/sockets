#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

// Shortcut for sending messages to users
#define SEND_M(a, b) sendto(socket, a, strlen(a), MSG_WAITALL, (struct sockaddr*)&b, sizeof(b))
#define MESSAGE_SIZE 256
#define LOGIN_SIZE 30
#define PORT 8080
#define TRUE 1

#define CONNECT_CODE 27
#define SEND_CODE 54
#define DISCONNECT_CODE 81

static pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t fd_mutex = PTHREAD_MUTEX_INITIALIZER;

struct User {
    struct sockaddr_in m_address;
    char m_name[LOGIN_SIZE];
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

void rip() { while (waitpid(-1, NULL, WNOHANG) > 0); } // Use for clean zombie

void* create_shared_memory(size_t size) { return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); }

void add_user(struct sockaddr_in client_address, char* message, int socket)
{
    pthread_mutex_lock(&user_mutex);
	
    if (data->size == data->capacity) {
        munmap(data->user_list, sizeof(*data->user_list) * data->capacity);
		data->capacity *= 2;
        data->user_list = create_shared_memory(sizeof(*data->user_list) * data->capacity);
	}

    strcpy(data->user_list[data->size].m_name, message);
    memcpy(&data->user_list[data->size].m_address, &client_address, sizeof(client_address));
    data->size++;
    
    pthread_mutex_unlock(&user_mutex);

    pthread_mutex_lock(&fd_mutex); // Sending chat to new user
    
    int fd = open("chat.txt", O_CREAT | O_RDONLY, S_IRWXU);
    if (fd < 0) { perror(""), exit(EXIT_FAILURE); }
    
    char line[MESSAGE_SIZE];
    memset(&line, 0, MESSAGE_SIZE);
    
    for(int i = 0; read(fd, &line[i], 1) > 0; i++) {
        if (line[i] == '\n' || i + 1 == MESSAGE_SIZE) { // Sending each message separately
            line[i + 1] = '\0';           // We can't clean buf couse send_m  
            SEND_M(line, client_address); // isnt a blocking operation so we assign last char '\0'
            i = -1; // Couse action at the end of loop will increase i on 1
        }
    }
    close(fd);
    
    pthread_mutex_unlock(&fd_mutex);
    printf("SERVER:\033[1m\033[32mCONNECT\033[0m %s\n", data->user_list[data->size - 1].m_name);
}
void send_message(int socket, char* message){

    pthread_mutex_lock(&fd_mutex);
    
    int fd = open("chat.txt", O_CREAT | O_WRONLY | O_APPEND, S_IRWXU);
    if (fd < 0) { perror(""), exit(EXIT_FAILURE); }
    write(fd, message, strlen(message)); // Add message to chat file for new users
    close(fd);
    
    pthread_mutex_unlock(&fd_mutex);

    for (size_t i = 0; i < data->size; i++) { SEND_M(message, data->user_list[i].m_address); }
}
void delete_user(char* message)
{
    size_t i = 0; // Geting index of user for his escape
    while(i != data->size && strcmp(data->user_list[i].m_name, message) != 0) { i++; }

    if (data->size == i) printf("UNKNOWN MEMBER %s\n", message), exit(EXIT_FAILURE);

    printf("SERVER:DISCONNECT %s\n", data->user_list[i].m_name);
    
    pthread_mutex_lock(&user_mutex);
    
    for (size_t j = i; j < data->size; j++) { // move users to the valid positions of list
        memset(data->user_list[j].m_name, 0, sizeof(data->user_list[j].m_name));
        strcpy(data->user_list[j].m_name, data->user_list[j + 1].m_name);

        memset(&data->user_list[j].m_address, 0, sizeof(data->user_list[j].m_address));
        memcpy(&data->user_list[j].m_address, &data->user_list[j + 1].m_address, sizeof(data->user_list[j + 1].m_address));
    }
    data->size--;
    pthread_mutex_unlock(&user_mutex);
}
void work(int socket, struct sockaddr_in client_address, char* message)
{
    switch((message[0] - '0') * 10 + (message[1] - '0')) {
        case CONNECT_CODE: add_user(client_address, message + 2, socket);
            break;
        case SEND_CODE: send_message(socket, message + 2);
            break;
        case DISCONNECT_CODE: delete_user(message + 2);
            break;
    }
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
	if ((data = create_shared_memory(sizeof(*data))) == NULL) { exit(EXIT_FAILURE); }
    data->size = 0, data->capacity = 50;
    if ((data->user_list = create_shared_memory(sizeof(*data->user_list) * data->capacity)) == NULL) { exit(EXIT_FAILURE); }

    int server_socket = check(socket(AF_INET, SOCK_DGRAM, 0), "SOCKET");

    struct sockaddr_in server_address, client_address;
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    if (argc == 2) {
        inet_aton(argv[1], &server_address.sin_addr);
    } else {
        inet_aton("192.168.0.5", &server_address.sin_addr);
    }
    check(bind(server_socket, (const struct sockaddr*)&server_address, sizeof(server_address)), "BIND");
    
    struct timeval read_timeout;
    read_timeout.tv_sec = 0, read_timeout.tv_usec = 10;
    setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));

    fd_set sockets; // When the server is empty and 10 seconds have passed, it will close
    FD_ZERO(&sockets);
    FD_SET(server_socket, &sockets);
    
    printf("MY IP %s\n", inet_ntoa(server_address.sin_addr));
    signal(SIGCHLD, rip);
    
    struct timeval timeout = { 5, 0 };
    for(char message[MESSAGE_SIZE]; TRUE; timeout.tv_sec = 5, timeout.tv_usec = 0, FD_SET(server_socket, &sockets)) {
        if (select(FD_SETSIZE, &sockets, NULL, NULL, &timeout) == 0) {
            if (data->size == 0) { break; }
        }
        memset(&message, 0, MESSAGE_SIZE), memset(&client_address, 0, sizeof(client_address));
        socklen_t addr_len = sizeof(client_address);
        if (recvfrom(server_socket, message, MESSAGE_SIZE, 0, (struct sockaddr*)&client_address, &addr_len) < 0) {
            if (errno != EAGAIN) { perror("SERVER:RECV"), exit(EXIT_FAILURE); }
        } else {
            if (fork() == 0) { work(server_socket, client_address, message); }
        }        
    }
    munmap(data->user_list, sizeof(*data->user_list) * data->capacity);
    munmap(data, sizeof(*data));

    check(close(server_socket), "SHUT DOWN");
    return 0;
}