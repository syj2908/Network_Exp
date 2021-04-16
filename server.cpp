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

int conn_fd[QUE_NUM];   //所有用户的套接字
using namespace std;

/*
    V1.3    2021/4/16
        *编写并测试了一个简单CSC聊天模型所需要的各种功能
        **下一目标：用户验证
        
*/

struct INFO
{
    //传递给常驻recv线程的所有参数
    int sock_fd;    //套接字
    int NO; //编号
};

struct INFO_SEND
{
    //传递给send线程的所有参数
    int dst_sock_fd;    //目的用户套接字
    int NO; //编号
    char buffer[MAX_BUFF_LEN];  //待发送内容
};

void *send_func(void *arg)
{
    INFO_SEND *info_send = (INFO_SEND *)arg;

    if (send(info_send->dst_sock_fd, info_send->buffer, sizeof(info_send->buffer), 0) == -1)
    {
        cout << "send message fail" << endl;
        pthread_exit(0);
    }
    else
    {
        cout << "MESSAGE SEND TO " << (info_send->NO == 0 ? 1 : 0) << ": " << info_send->buffer;
    }

    pthread_exit(0);
}

void *recv_func(void *arg)
{
    INFO *info = (INFO *)arg;
    pthread_t send_thread0,send_thread1;
    char recv_buffer[MAX_BUFF_LEN];
    int send_result;
    INFO_SEND info_send;

    while (1)
    {
        if (recv(info->sock_fd, recv_buffer, sizeof(recv_buffer), 0) > 0)
        {
            cout << "MESSAGE RECV FROM NO." << info->NO << ": " << recv_buffer;

            info_send.NO = info->NO;
            info_send.dst_sock_fd = (info->NO == 0) ? conn_fd[1] : conn_fd[0];
            strcpy(info_send.buffer, recv_buffer);

            if(info->NO==0)
            {
                send_result = pthread_create(&send_thread0, NULL, send_func, &info_send);
            }
            else if(info->NO==1)
            {
                send_result = pthread_create(&send_thread1, NULL, send_func, &info_send);
            }
        }
        if(info->NO==0)
        {
            send_result = pthread_join(send_thread0, NULL);
        }
        else if(info->NO==1)
        {
            send_result = pthread_join(send_thread1, NULL);
        }
        if (strncmp(recv_buffer, "quit", 4) == 0)
            break;
    }
}

int main()
{
    pthread_t recv_thread0, recv_thread1;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    int recv_result[QUE_NUM];
    int link_num = 0;   //已连接人数
    struct sockaddr_in clint_addr[QUE_NUM]; //客户端地址数组
    INFO info[QUE_NUM];    //传递参数的数组

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
    
        info[link_num].sock_fd = conn_fd[link_num];
        info[link_num].NO = link_num;

        if(link_num==0)
        {
            recv_result[link_num] = pthread_create(&recv_thread0, NULL, recv_func, &info[link_num]);
        }
        else if(link_num==1)
        {
            recv_result[link_num] = pthread_create(&recv_thread1, NULL, recv_func, &info[link_num]);
        }

        if (recv_result[link_num] != 0)
        {
            cout << "recv_thread create fail" << endl;
            cout << errno << endl;
            exit(1);
        }
    }

    recv_result[0] = pthread_join(recv_thread0, NULL);
    recv_result[1] = pthread_join(recv_thread1, NULL);
    
    cout << "SYSTEM: Chat over." << endl;

    for (int i = 0; i < 2;i++)
        close(conn_fd[i]);
    close(sock_fd);
    return 0;
}