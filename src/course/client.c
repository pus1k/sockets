#include <arpa/inet.h>
#include <errno.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define SEND_M(a) sendto(data.m_socket, a, strlen(a), MSG_WAITALL, (struct sockaddr*)&data.m_saddr, sizeof(data.m_saddr))
#define MESSAGE_SIZE 256
#define LOGIN_SIZE 30
#define PORT 8080

struct client_app_data {
    struct winsize m_ws;
    struct sockaddr_in m_saddr, m_caddr;
    size_t m_x, m_chat_capacity, m_chat_size, m_msg_size;
    size_t m_up, m_down;
    int m_socket;
    char** m_chat;
    char m_msg[MESSAGE_SIZE];
    char m_rcv[MESSAGE_SIZE];
    char m_login[LOGIN_SIZE];
} data;

int check(int result, char* msg)
{
    if (result < 0) {
        char buff[500];
        sprintf(buff, "CLIENT:\033[1m\033[31m%s\033[0m", msg), perror(buff), exit(EXIT_FAILURE);
    }
    printf("CLIENT:\033[1m\033[32m%s\033[0m\n", msg);
    return result;
}
void __init__(int argc, char* argv[])
{   // CREATE CLIENT SOCKET
    data.m_socket = check(socket(AF_INET, SOCK_DGRAM, 0), "SOCKET");

    memset(&data.m_saddr, 0, sizeof(data.m_saddr)), memset(&data.m_caddr, 0, sizeof(data.m_caddr));
    data.m_saddr.sin_family = AF_INET;
    data.m_saddr.sin_port = htons(PORT);
    if (argc == 2) {
        inet_aton(argv[1], &data.m_saddr.sin_addr);
    } else {
        inet_aton("192.168.0.5", &data.m_saddr.sin_addr);
    }
    
    check(bind(data.m_socket, (const struct sockaddr*)&data.m_caddr, sizeof(data.m_caddr)), "BIND");

    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 10;
    setsockopt(data.m_socket, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));
    
    // INIT CLIENT DATA
    ioctl(fileno(stdout), TIOCGWINSZ, (char*)&data.m_ws);
    memset(&data.m_msg, 0, sizeof(data.m_msg)), memset(&data.m_login, 0, sizeof(data.m_login));
    memset(&data.m_rcv, 0, sizeof(data.m_rcv)), data.m_msg_size = 0, data.m_x = 4;
    data.m_up = 0, data.m_down = 0;
    data.m_chat_capacity = 1000, data.m_chat_size = 0;
    data.m_chat = malloc(sizeof(*data.m_chat) * data.m_chat_capacity);
    if (data.m_chat == NULL) { perror("CLIENT:CALLOC"), exit(EXIT_FAILURE); }
}
void draw()
{
    WINDOW* chat_win = newwin(data.m_ws.ws_row - 2, data.m_ws.ws_col, 0, 0);
    WINDOW* enter_win = newwin(3, data.m_ws.ws_col, data.m_ws.ws_row - 2, 0);
    mvwprintw(enter_win, 1, 2, ": %s", data.m_msg);
    
    wprintw(chat_win, "\n");
    for (size_t i = data.m_up; i < data.m_down; i++)
        wprintw(chat_win, "  %s", data.m_chat[i]);

    box(enter_win, 0, 0);
    box(chat_win, 0, 0);
    move(data.m_ws.ws_row - 1 , data.m_x);
    wrefresh(chat_win);
    wrefresh(enter_win);
    refresh();
    delwin(chat_win);
    delwin(enter_win);
}
void add_symbol_to_message(const int ch)
{
    if (data.m_msg_size + 6 < data.m_ws.ws_col) {
        if (data.m_x == 4) {
            memcpy(&data.m_msg[1], &data.m_msg[0], data.m_msg_size);
        } else if (data.m_x < data.m_msg_size + 4) {
            memcpy(&data.m_msg[data.m_x - 3], &data.m_msg[data.m_x - 4], data.m_msg_size - data.m_x + 4);
        }
        data.m_msg[-4 + data.m_x++] = (char)ch;
        data.m_msg_size++;
    }
}
void send_message(char* msg, int code)
{
    char *buffer = calloc(sizeof(char), 500);
    buffer[0] = '0' + code / 10, buffer[1] = '0' + code % 10;
    if (code == 54) {
        memcpy(&buffer[2], data.m_login, strlen(data.m_login));
        buffer[2 + strlen(data.m_login)] = ':';
        memcpy(&buffer[2 + strlen(data.m_login) + 1], msg, strlen(msg));
        buffer[strlen(buffer)] = '\n';
    } else if (code == 81 || code == 27) { 
        memcpy(&buffer[2], data.m_login, strlen(data.m_login));
    }
    while (SEND_M(buffer) == -1);
    free(buffer);
}
void add_new_msg_to_chat()
{
    if (data.m_chat_size == data.m_chat_capacity) {
        data.m_chat_capacity *= 2;
        char** tmp = malloc(sizeof(char*) * data.m_chat_capacity);
        if (tmp == NULL) { perror("CLIENT:REALLOC"), exit(EXIT_FAILURE); }
        memcpy(tmp, data.m_chat, data.m_chat_size);
        free(data.m_chat);
        data.m_chat = tmp;
    }
    
    data.m_chat[data.m_chat_size] = calloc(sizeof(*data.m_chat[data.m_chat_size]), strlen(data.m_rcv));
    if (data.m_chat[data.m_chat_size] == NULL) { perror("CLIENT:CALLOC"), exit(EXIT_FAILURE); }
    
    strcpy(data.m_chat[data.m_chat_size++], data.m_rcv);

    if (data.m_ws.ws_row > data.m_chat_size + 4) {
        data.m_down = data.m_chat_size;
    } else {
        data.m_up = data.m_chat_size - data.m_ws.ws_row + 4;
        data.m_down = data.m_chat_size;
    }
}

void login()
{
    WINDOW* login_win = newwin(5, 36, data.m_ws.ws_row / 2 - 3, data.m_ws.ws_col / 2 - 15);
    mvwprintw(login_win, 1, getmaxx(login_win) / 2 - 8, "ENTER YOUR LOGIN\n ");
    box(login_win, 0, 0);
    wrefresh(login_win);
    wgetstr(login_win, data.m_login);
    delwin(login_win);
    send_message(NULL, 27);
    draw();
}
int main(int argc, char* argv[])
{
    __init__(argc, argv);
    initscr();
    keypad(stdscr, TRUE);
    login();

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    struct timeval timeout = { 0, 10 };
    
    for (int ch = 0; ch != KEY_END; timeout.tv_usec = 10, FD_SET(0, &fds)) {
        if (select(FD_SETSIZE, &fds, NULL, NULL, &timeout) == 0) {
            socklen_t length = sizeof(data.m_saddr);
            memset(&data.m_rcv, 0, sizeof(data.m_rcv));
            if (recvfrom(data.m_socket, data.m_rcv, MESSAGE_SIZE, 0, (struct sockaddr*)&data.m_saddr, &length) < 0) {
                if (errno != EAGAIN) { 
                    endwin(), perror("CLIENT:RECV"), exit(EXIT_FAILURE);
                }
            } else {
                add_new_msg_to_chat();
            }    
        } else {
            ch = getch();
            switch (ch) {
                case KEY_RIGHT: if (data.m_x < data.m_msg_size + 4) data.m_x++;
                    break;
                case KEY_LEFT: if (data.m_x > 4) data.m_x--;
                    break;
                case KEY_UP: if (data.m_up > 0) data.m_up--, data.m_down--;
                    break;
                case KEY_DOWN: if (data.m_down < data.m_chat_size) data.m_up++, data.m_down++;
                    break;
                case KEY_BACKSPACE:
                    if (data.m_x > 4) {
                        data.m_x--;
                        memcpy(&data.m_msg[data.m_x - 4], &data.m_msg[data.m_x - 3], data.m_msg_size - data.m_x + 4);
                        data.m_msg[data.m_msg_size--] = '\0';
                    }
                    break;
                case 10:
                    send_message(data.m_msg, 54);
                    memset(data.m_msg, 0, sizeof(data.m_msg)), data.m_msg_size = 0, data.m_x = 4;
                    break;
                default:
                    if (ch > 31 && ch < 127) add_symbol_to_message(ch);
            }
        }
        draw();
    }
    endwin();
    for (size_t i = 0; i < data.m_chat_size; i++) { free(data.m_chat[i]); }
    free(data.m_chat);
    send_message(NULL, 81);
    check(close(data.m_socket), "EXIT");
    return 0;
}