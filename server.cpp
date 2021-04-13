#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>    //g++ -pthread -o server server.cpp
#include <string.h>

#define IP "192.168.134.128" //服务器IP
#define PORT 8000            //服务器端口号
#define QUE_NUM 20           //最大连接数
#define MAX_BUFF_LEN 1024    //最大缓冲区长度

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
                cout << "MESSAGE SEND: " << send_buffer << endl;
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
            cout << "MESSAGE RECV: " << recv_buffer << endl;
        }

        if (strncmp(recv_buffer, "quit", 4) == 0)
            break;
    }
}

int main()
{
    pthread_t send_thread, recv_thread;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    int conn_fd;
    int link_num = 0;   //已连接人数
    struct sockaddr_in clint_addr[QUE_NUM]; //客户端地址数组

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP);

 

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        cout << "bind fail" << endl;
        cout << errno << endl;
        exit(1);
    }

    if (listen(sock_fd, QUE_NUM) == -1)
    {
        cout << "listen fail" << endl;
        exit(1);
    }

    cout << "Waiting for Connect..." << endl;

    while (link_num < QUE_NUM)
    {
        socklen_t clint_addr_size = sizeof(clint_addr[link_num]);
        if ((conn_fd = accept(sock_fd, (struct sockaddr *)&clint_addr[link_num], &clint_addr_size)) == -1)
        {
            cout << "accept fail" << endl;
            exit(1);
        }
        else
        {
            cout << "User " << inet_ntoa(clint_addr[link_num].sin_addr) << " Has Connect!" << endl;
            link_num++;
        }
    }

    int send_result = pthread_create(&send_thread, NULL, send_func, (void *)(long)conn_fd);
    int recv_result = pthread_create(&recv_thread, NULL, recv_func, (void *)(long)conn_fd);

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

    close(conn_fd);
    close(sock_fd);
    return 0;
}