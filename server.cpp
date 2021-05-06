#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h> //g++ -pthread -o server server.cpp
#include <string.h>

#define IP "10.172.81.27" //服务器IP
#define PORT 8000           //服务器端口号
#define ftp_PORT 8006       //文件传输端口号
#define voice_PORT 8008     //语音聊天
#define QUE_NUM 2           //最大连接数
#define MAX_BUFF_LEN 1024   //最大缓冲区长度

int conn_fd[QUE_NUM];   //所有用户的套接字
int ftp_fd[QUE_NUM];    //文件传输用的套接字
int sender;             //文件发送方info->NO

using namespace std;

/*
    V1.4    2021/4/21
        *编写并测试了一个简单CSC聊天模型所需要的各种功能
        *实现了服务器端的用户验证功能，但还需要和客户端对接口令的传输格式

*/

struct INFO
{
    //传递给常驻recv线程的所有参数
    int sock_fd; //套接字
    int NO;      //编号
};

struct INFO_SEND
{
    //传递给send线程的所有参数
    int dst_sock_fd;           //目的用户套接字
    int NO;                    //编号
    char buffer[MAX_BUFF_LEN]; //待发送内容
};

char *format(char str[], int n)
{
    //删除fgets得到的字符串的末尾换行符

    for (int i = 0; i < n; i++)
    {
        if (str[i] == '\n')
        {
            str[i] = '\0';
        }
    }
    return str;
}

void ID_verify(int sock_fd)
{
    char Key[100] = {0};
    char passwd[] = "syj 123";
    char success[] = "1";
    char fail[] = "-1";
    while (1)
    {
        if (recv(sock_fd, Key, sizeof(Key), 0) > 0)
        {
            if (strcmp(Key, passwd) != 0)
            {
                format(Key, sizeof(Key));
                cout << "User " << Key << " is trying to login." << endl;
                send(sock_fd, fail, (int)strlen(fail), 0);
            }
            else
                break;
        }
    }

    cout << "User " << Key << " has login." << endl;
    send(sock_fd, success, (int)strlen(success), 0);
}

void *send_func(void *arg)
{
    INFO_SEND *info_send = (INFO_SEND *)arg;

    if (send(info_send->dst_sock_fd, info_send->buffer, (int)strlen(info_send->buffer), 0) == -1)
    {
        cout << "send message fail" << endl;
        pthread_exit(0);
    }
    else
    {
        cout << "MESSAGE SEND TO " << (info_send->NO == 0 ? 1 : 0) << ": " << info_send->buffer << endl;
    }

    pthread_exit(0);
}

void ftp_offline(int sock_fd)
{
    char filename[100] = "test.mp4";  //文件名
    FILE *fp = fopen(filename, "wb"); //以二进制方式打开（创建）文件
    char buffer[MAX_BUFF_LEN] = {0};  //文件缓冲区
    int nCount;
    cout << "start1" << endl;
    while (1)
    {
        recv(sock_fd, buffer, MAX_BUFF_LEN, 0);
        cout << buffer << endl;
        if (strncmp(buffer, "ftpstart", 8) == 0)
        {
            cout << "strat2" << endl;
            break;
        }
    }
    cout << "start3" << endl;
    while (1)
    {
        nCount = recv(sock_fd, buffer, MAX_BUFF_LEN, 0);
        //cout<<nCount<<endl;
        if (strncmp(buffer, "ftpfin", 6) == 0)
        {
            cout << "传输完毕" << endl;
            break;
        }

        if (nCount > 0)
        {
            fwrite(buffer, nCount, 1, fp);
        }
    }
    fclose(fp);
    char success[] = "File transfer success!";
    cout << "File transfer success!" << endl;
    send(sock_fd, success, sizeof(success), 0);
}

void ftp_online(int FTP_SEND)
{
    int FTP_RECV = 1 - FTP_SEND;
    char ftpreq[] = "ftpreq";

    send(ftp_fd[FTP_RECV], ftpreq, (int)strlen(ftpreq), 0);

    char ftpstart[] = "ftpstart";
    char ftp_buffer[MAX_BUFF_LEN];

    while (1)
    {
        recv(ftp_fd[FTP_SEND], ftp_buffer, MAX_BUFF_LEN, 0);
        if (strncmp(ftp_buffer, "ftpstart", 8) == 0)
            break;
    }
    int test;
    test = send(ftp_fd[FTP_RECV], ftpstart, (int)strlen(ftpstart), 0);
    cout << test << "开始" << endl;
    while (1)
    {
        test = recv(ftp_fd[FTP_SEND], ftp_buffer, MAX_BUFF_LEN, 0);
        cout << "收" << test << endl;
        if (strncmp(ftp_buffer, "ftpfin", 6) == 0)
            break;
        test = send(ftp_fd[FTP_RECV], ftp_buffer, test, 0);
        cout << "发" << test << endl;
    }
    char success[] = "File transfer success!";
    send(ftp_fd[FTP_SEND], success, (int)strlen(success), 0);
    char fin[] = "ftpfin";
    sleep(5 * 100);
    send(ftp_fd[FTP_RECV], fin, (int)strlen(fin), 0);
    cout << "File transfer success!" << endl;
}

void *waiting_func(void *arg)
{
    INFO *info = (INFO *)arg;
    char recv_buffer[MAX_BUFF_LEN];
    
    if(recv(info->sock_fd, recv_buffer, sizeof(recv_buffer), 0) > 0)
    {
        if(strncmp(recv_buffer, "I am sender.", 10) == 0)
        {
            sender = info->NO;
        }
    }
    pthread_exit(0);
}

void *ftp_func(void *arg)
{
    int cmd = *(int *)arg;
    int link_num = 0;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in client_addr[QUE_NUM];
    INFO info[QUE_NUM];
    pthread_t waiting_thread0, waiting_thread1;
    int waiting_result[QUE_NUM];

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ftp_PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP);

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        cout << "ftp bind fail" << endl;
        cout << errno << endl;
        exit(1);
    }

    if (listen(sock_fd, QUE_NUM) == -1)
    {
        cout << "ftp listen fail" << endl;
        exit(1);
    }

    cout << "preparing for file transmission..." << endl;

    for (link_num = 0; link_num < QUE_NUM;link_num++)
    {
        socklen_t client_addr_size = sizeof(client_addr[link_num]);
        if ((ftp_fd[link_num] = accept(sock_fd, (struct sockaddr *)&client_addr[link_num], &client_addr_size)) == -1)
        {
            cout << "ftp accept fail" << endl;
            exit(1);
        }

        cout << "User " << inet_ntoa(client_addr[link_num].sin_addr) << " is ready to transmit file!" << endl;

        info[link_num].sock_fd = ftp_fd[link_num];
        info[link_num].NO = link_num;
    }

    for (int i = 0; i < QUE_NUM;i++)
    {
        char signal[] = "file transmission ready.";
        send(ftp_fd[i], signal, (int)strlen(signal), 0);
    }

    waiting_result[0] = pthread_create(&waiting_thread0, NULL, waiting_func, &info[0]);
    waiting_result[1] = pthread_create(&waiting_thread1, NULL, waiting_func, &info[1]);

    waiting_result[0] = pthread_join(waiting_thread0, NULL);
    waiting_result[1] = pthread_join(waiting_thread1, NULL);

    if (cmd == 1)
    {
        ftp_offline(info[sender].sock_fd);
    }
    else if (cmd == 2)
    {
        ftp_online(sender);
    }

    cout << "File transmission success!" << endl;

    for (int i = 0; i < 2; i++)
        close(ftp_fd[i]);
    close(sock_fd);
    pthread_exit(0);
}

void *recv_func(void *arg)
{
    INFO *info = (INFO *)arg;
    pthread_t send_thread, ftp_thread;
    char recv_buffer[MAX_BUFF_LEN];
    int send_result;
    INFO_SEND info_send;

    ID_verify(conn_fd[info->NO]);

    while (1)
    {

        if (recv(info->sock_fd, recv_buffer, sizeof(recv_buffer), 0) > 0)
        {
            cout << "MESSAGE RECV FROM NO." << info->NO << ": " << recv_buffer << endl;

            if ((strncmp(recv_buffer, "FTPoffline", 10) == 0) || (strncmp(recv_buffer, "FTPonline", 9)== 0))
            {
                int cmd;
                if(strncmp(recv_buffer, "FTPoffline", 10) == 0)
                {
                    cmd = 1;
                }
                else if (strncmp(recv_buffer, "FTPonline", 9) == 0)
                {
                    cmd = 2;
                }
                int ftp_result = pthread_create(&ftp_thread, NULL, ftp_func, &cmd);
                continue;
            }
            if (strncmp(recv_buffer, "quit", 4) == 0)
            {
                break;
            }

            info_send.NO = info->NO;
            info_send.dst_sock_fd = (info->NO == 0) ? conn_fd[1] : conn_fd[0];
            strcpy(info_send.buffer, recv_buffer);

            send_result = pthread_create(&send_thread, NULL, send_func, &info_send);
        }
        send_result = pthread_join(send_thread, NULL);
    }
}

int main()
{
    pthread_t recv_thread0, recv_thread1;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    int recv_result[QUE_NUM];
    int link_num = 0;                       //已连接人数
    struct sockaddr_in client_addr[QUE_NUM]; //客户端地址数组
    INFO info[QUE_NUM];                     //传递参数的数组

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
        socklen_t client_addr_size = sizeof(client_addr[link_num]);
        if ((conn_fd[link_num] = accept(sock_fd, (struct sockaddr *)&client_addr[link_num], &client_addr_size)) == -1)
        {
            cout << "accept fail" << endl;
            exit(1);
        }

        cout << "User " << inet_ntoa(client_addr[link_num].sin_addr) << " Has Connect!" << endl;

        info[link_num].sock_fd = conn_fd[link_num];
        info[link_num].NO = link_num;

        if (link_num == 0)
        {
            recv_result[link_num] = pthread_create(&recv_thread0, NULL, recv_func, &info[link_num]);
        }
        else if (link_num == 1)
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

    for (int i = 0; i < 2; i++)
        close(conn_fd[i]);
    close(sock_fd);
    return 0;
}