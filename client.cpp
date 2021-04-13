#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>

#define SERVER_PORT 8000
#define SERVER_IP "192.168.134.128"
#define MAX_BUFF_LEN 1024

using namespace std;

void *send_func(void *arg)
{
    int sock_fd = (int)(long)arg;
    char send_buffer[MAX_BUFF_LEN];
    while (1)
    {
        if (fgets(send_buffer, MAX_BUFF_LEN, stdin) == NULL)
        {
            continue;
        }
        else
        {
            if (send(sock_fd, send_buffer, sizeof(send_buffer), 0) == -1)
            {
                cout << "send message fail" << endl;
                continue;
            }
            else
            {
                cout << endl;
                cout << "MESSAGE SEND: " << send_buffer << endl;
                cout << "-->";
            }

            if (strncmp(send_buffer, "quit", 4) == 0)
                break;
        }
    }
    pthread_exit(0);
}

void *recv_func(void *arg)
{
    int sock_fd = (int)(long)arg;
    char recv_buffer[MAX_BUFF_LEN];
    while (1)
    {
        if (recv(sock_fd, recv_buffer, sizeof(recv_buffer), 0) > 0)
        {
            cout << endl;
            cout << "MESSAGE RECV: " << recv_buffer << endl;
            cout << "-->";
        }

        if (strncmp(recv_buffer, "quit", 4) == 0)
            break;
    }
}

int main()
{
    pthread_t send_thread, recv_thread;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    int rec_len;
    char send_buffer[MAX_BUFF_LEN];
    char recv_buffer[MAX_BUFF_LEN];
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        cout << "connect fail" << endl;
        exit(1);
    }
    else
    {
        cout << "   ---ChatRoom Linked---   " << endl;
    }

    int send_result = pthread_create(&send_thread, NULL, send_func, (void *)(long)sock_fd);
    int recv_result = pthread_create(&recv_thread, NULL, recv_func, (void *)(long)sock_fd);

    if (send_result != 0)
    {
        cout << "send_thread create fail" << endl;
        exit(1);
    }
    if (recv_result != 0)
    {
        cout << "recv_thread create fail" << endl;
        exit(1);
    }

    send_result = pthread_join(send_thread, NULL);
    recv_result = pthread_join(recv_thread, NULL);

    cout << "SYSTEM: Chat over." << endl;

    close(sock_fd);
    return 0;
}