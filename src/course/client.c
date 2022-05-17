#include <arpa/inet.h>
#include <errno.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define SEND_M(a) sendto(data.socket, a, strlen(a), MSG_WAITALL, (struct sockaddr*)&data.saddr, sizeof(data.saddr))
#define MESSAGE_SIZE 256
#define LOGIN_SIZE 20
#define PORT 8080

struct client_app_data {
    struct winsize ws;
    struct sockaddr_in saddr, caddr;
    size_t x, chat_capacity, chat_size, msg_size;
    size_t up, down;
    int socket;
    size_t offset;
    char** chat;
    char msg[MESSAGE_SIZE];
    char rcv[MESSAGE_SIZE];
    char login[LOGIN_SIZE];
} data;

void send_message(char* msg, int code);

int check(int result, char* msg)
{
    if (result < 0) {
        char buff[500];
        sprintf(buff, "CLIENT:\033[1m\033[31m%s\033[0m", msg), perror(buff), exit(EXIT_FAILURE);
    }
    printf("CLIENT:\033[1m\033[32m%s\033[0m\n", msg);
    return result;
}
void __exit__(short exit_statuts)
{
    endwin();
    send_message(NULL, 81);
    check(close(data.socket), "EXIT");
    for (size_t i = 0; i < data.chat_size; i++) { free(data.chat[i]); }
    free(data.chat);
    exit(exit_statuts);
}
void __init__(int argc, char* argv[])
{   // CREATE CLIENT SOCKET
    data.socket = check(socket(AF_INET, SOCK_DGRAM, 0), "SOCKET");
    
    memset(&data.saddr, 0, sizeof(data.saddr)), memset(&data.caddr, 0, sizeof(data.caddr));
    data.saddr.sin_family = AF_INET;
    data.saddr.sin_port = htons(PORT);

    if (argc == 2) {
        inet_aton(argv[1], &data.saddr.sin_addr);
    } else {
        system("ip -4 a | grep global | grep -o '[0-9]\\+' >> info.txt");
        FILE* fp = fopen("info.txt", "r");
        char ip_string[16];
        char line[5];
        memset(&ip_string, 0, sizeof(ip_string));
        for (int i = 0; i < 4; i++) {
            fgets(line, sizeof(line), fp);
            line[strlen(line) - 1] = '.';
            strcpy(ip_string + strlen(ip_string), line);
        }
        ip_string[strlen(ip_string) - 1] = '\0';
        system("rm info.txt");
        inet_aton(ip_string, &data.saddr.sin_addr);
        fclose(fp);
    }
    
    check(bind(data.socket, (const struct sockaddr*)&data.caddr, sizeof(data.caddr)), "BIND");

    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 10;
    setsockopt(data.socket, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));
    
    // INIT CLIENT DATA
    ioctl(fileno(stdout), TIOCGWINSZ, (char*)&data.ws);
    memset(&data.msg, 0, sizeof(data.msg)), memset(&data.rcv, 0, sizeof(data.rcv));
    memset(&data.login, 0, sizeof(data.login)), data.msg_size = 0;

    data.up = 0, data.down = 0;
    data.chat_capacity = 1000, data.chat_size = 0;
    data.chat = malloc(sizeof(*data.chat) * data.chat_capacity);
    if (data.chat == NULL) { perror("CLIENT:CALLOC"), __exit__(EXIT_FAILURE); }
    initscr();
    keypad(stdscr, TRUE);
}

void draw()
{
    WINDOW* chat_win = newwin(data.ws.ws_row - 2, data.ws.ws_col, 0, 0);
    WINDOW* enter_win = newwin(3, data.ws.ws_col, data.ws.ws_row - 2, 0);
    
    mvwprintw(enter_win, 1, 2, "%s:%s", data.login, data.msg);
    wprintw(chat_win, "\n");
    for (size_t i = data.up; i < data.down; i++)
        wprintw(chat_win, "  %s", data.chat[i]);

    box(enter_win, 0, 0);
    box(chat_win, 0, 0);
    move(data.ws.ws_row - 1 , data.x);
    wrefresh(chat_win);
    wrefresh(enter_win);
    refresh();
    delwin(chat_win);
    delwin(enter_win);
    
}
void add_symbol_to_message(const int ch)
{
    if (data.msg_size + data.offset + 2 < data.ws.ws_col) {
        if (data.x == data.offset) {
            memcpy(&data.msg[1], &data.msg[0], data.msg_size);
        } else if (data.x < data.msg_size + data.offset) {
            memcpy(&data.msg[data.x - data.offset + 1], &data.msg[data.x - data.offset], data.msg_size - data.x + data.offset);
        }
        data.msg[-data.offset + data.x++] = (char)ch;
        data.msg_size++;
    }
}
void send_message(char* msg, int code)
{
    char *buffer = calloc(sizeof(char), 500);
    buffer[0] = '0' + code / 10, buffer[1] = '0' + code % 10;
    if (code == 54) {
        memcpy(&buffer[2], data.login, strlen(data.login));
        buffer[2 + strlen(data.login)] = ':';
        memcpy(&buffer[2 + strlen(data.login) + 1], msg, strlen(msg));
        buffer[strlen(buffer)] = '\n';
    } else if (code == 81 || code == 27) { 
        memcpy(&buffer[2], data.login, strlen(data.login));
    }
    if (SEND_M(buffer) == -1) { 
        __exit__(EXIT_FAILURE);
    }
    free(buffer);
}
void add_new_msg_to_chat()
{
    if (data.chat_size == data.chat_capacity) {
        data.chat_capacity *= 2;
        
        char** tmp = malloc(sizeof(char*) * data.chat_capacity);
        if (tmp == NULL) { perror("CLIENT:REALLOC"), __exit__(EXIT_FAILURE); }
        
        memcpy(tmp, data.chat, data.chat_size);
        free(data.chat);
        data.chat = tmp;
    }
    
    data.chat[data.chat_size] = calloc(sizeof(*data.chat[data.chat_size]), strlen(data.rcv));
    if (data.chat[data.chat_size] == NULL) { perror("CLIENT:CALLOC"), __exit__(EXIT_FAILURE); }
    
    strcpy(data.chat[data.chat_size++], data.rcv);

    if (data.ws.ws_row > data.chat_size + 4) {
        data.down = data.chat_size;
    } else {
        data.up = data.chat_size - data.ws.ws_row + 4;
        data.down = data.chat_size;
    }
}

void login()
{
    do {
        WINDOW* box_win = newwin(3, 36, data.ws.ws_row / 2 - 1, data.ws.ws_col / 2 - 15);
        WINDOW* login_win = newwin(1, 34, data.ws.ws_row / 2, data.ws.ws_col / 2 - 14);
        box(box_win, 0, 0);
        wrefresh(box_win);
        mvwprintw(login_win, 0, 1, "LOGIN: ");
        wrefresh(login_win);
        wgetstr(login_win, data.login);
        delwin(login_win);
    } while (strlen(data.login) < 4 || strlen(data.login) > 20);
    send_message(NULL, 27);
    data.offset = strlen(data.login) + 3;
    data.x = data.offset;
    draw();
}
int main(int argc, char* argv[])
{
    __init__(argc, argv);
    login();

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    struct timeval timeout = { 0, 10 };
    
    for (int ch = 0; ch != KEY_END; timeout.tv_usec = 10, FD_SET(0, &fds)) {
        if (select(FD_SETSIZE, &fds, NULL, NULL, &timeout) == 0) {
            socklen_t length = sizeof(data.saddr);
            memset(&data.rcv, 0, sizeof(data.rcv));
            if (recvfrom(data.socket, data.rcv, MESSAGE_SIZE, 0, (struct sockaddr*)&data.saddr, &length) < 0) {
                if (errno != EAGAIN) { 
                    endwin(), perror("CLIENT:RECV"), __exit__(EXIT_FAILURE);
                }
            } else {
                add_new_msg_to_chat();
            }    
        } else {
            ch = getch();
            switch (ch) {
                case KEY_RIGHT: if (data.x < data.msg_size + data.offset) data.x++;
                    break;
                case KEY_LEFT: if (data.x > data.offset) data.x--;
                    break;
                case KEY_UP: if (data.up > 0) data.up--, data.down--;
                    break;
                case KEY_DOWN: if (data.down < data.chat_size) data.up++, data.down++;
                    break;
                case KEY_BACKSPACE:
                    if (data.x > data.offset) {
                        data.x--;
                        memcpy(&data.msg[data.x - data.offset], &data.msg[data.x - data.offset + 1], data.msg_size - data.x + data.offset);
                        data.msg[data.msg_size--] = '\0';
                    }
                    break;
                case 10:
                    send_message(data.msg, 54);
                    memset(data.msg, 0, sizeof(data.msg)), data.msg_size = 0, data.x = data.offset;
                    break;
                default:
                    if (ch > 31 && ch < 127) add_symbol_to_message(ch);
            }
        }
        draw();
    }
    __exit__(EXIT_SUCCESS);
    return 0;
}