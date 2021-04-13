#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>    //g++ -pthread -o server server.cpp
#include <string.h>

#define IP "192.168.140.128" //服务器IP
#define PORT 8000            //服务器端口号
#define QUE_NUM 2           //最大连接数
#define MAX_BUFF_LEN 1024    //最大缓冲区长度

using namespace std;

/*
    V1.2    2021/4/13
        *允许多人连接到服务器
        *服务器能收到来自各个用户的消息
        *但是服务器只能发消息到某一用户
        *用户断开连接过程也有问题
*/

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
    int conn_fd[QUE_NUM];
    int send_result[QUE_NUM];
    int recv_result[QUE_NUM];
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

    for (link_num = 0; link_num < 2; link_num++)
    {
        socklen_t clint_addr_size = sizeof(clint_addr[link_num]);
        if ((conn_fd[link_num] = accept(sock_fd, (struct sockaddr *)&clint_addr[link_num], &clint_addr_size)) == -1)
        {
            cout << "accept fail" << endl;
            exit(1);
        }
        
        cout << "User " << inet_ntoa(clint_addr[link_num].sin_addr) << " Has Connect!" << endl;

        // int temp_fd = conn_fd[link_num];
        send_result[link_num] = pthread_create(&send_thread, NULL, send_func, (void *)(long)conn_fd[link_num]);
        recv_result[link_num] = pthread_create(&recv_thread, NULL, recv_func, (void *)(long)conn_fd[link_num]);

        if (send_result[link_num] != 0)
        {
            cout << "send_thread create fail" << endl;
            cout << errno << endl;
            exit(1);
        }
        if (recv_result[link_num] != 0)
        {
            cout << "recv_thread create fail" << endl;
            cout << errno << endl;
            exit(1);
        }
    }

    send_result[link_num] = pthread_join(send_thread, NULL);
    recv_result[link_num] = pthread_join(recv_thread, NULL);
    
    cout << "SYSTEM: Chat over." << endl;

    for (int i = 0; i < 2;i++)
        close(conn_fd[i]);
    close(sock_fd);
    return 0;
}