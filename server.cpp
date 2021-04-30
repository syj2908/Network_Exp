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
#define MAX_BUFF_LEN 1024  //最大缓冲区长度

int conn_fd[QUE_NUM];   //所有用户的套接字
using namespace std;

/*
    V1.4    2021/4/21
        *编写并测试了一个简单CSC聊天模型所需要的各种功能
        *实现了服务器端的用户验证功能，但还需要和客户端对接口令的传输格式

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

char *format(char str[],int n)
{
    //删除fgets得到的字符串的末尾换行符

    for (int i = 0; i < n;i++)
    {
        if(str[i]=='\n')
        {
            str[i] = '\0';
        }
    }
    return str;
}

void ID_verify(int sock_fd)
{
    char Key[100]={0};
    char passwd[] = "syj 123";
    char success[] = "1";
    char fail[] = "-1";
    while(1)
    {
        if (recv(sock_fd, Key, sizeof(Key), 0) > 0)
        {
            if(strcmp(Key,passwd)!=0)
            {
                format(Key,sizeof(Key));
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
        cout << "MESSAGE SEND TO " << (info_send->NO == 0 ? 1 : 0) << ": " << info_send->buffer<<endl;
    }

    pthread_exit(0);
}
void ftp_offline(int sock_fd)
{
    char filename[100] = "test.mp4";  //文件名
    FILE *fp = fopen(filename, "wb"); //以二进制方式打开（创建）文件
    char buffer[MAX_BUFF_LEN] = {0};  //文件缓冲区
    int nCount;
    cout<<"start1"<<endl;
    while(1)
    {
        recv(sock_fd, buffer, MAX_BUFF_LEN, 0);
        cout<<buffer<<endl;
        if(strncmp(buffer, "ftpstart", 8) == 0)
        {
            cout<<"strat2"<<endl;
            break;
        }
        
    }
    cout<<"start3"<<endl;
    while(1)
    {
        nCount = recv(sock_fd, buffer, MAX_BUFF_LEN, 0);
        //cout<<nCount<<endl;
        if(strncmp(buffer, "ftpfin", 6) == 0)
        {
            cout<<"传输完毕"<<endl;
            break;
        }
            
        if(nCount>0)
        {
             fwrite(buffer,nCount,1,fp);
        }
    }
    fclose(fp);
    char success[]="File transfer success!";
    cout<<"File transfer success!"<<endl;
    send(sock_fd, success, sizeof(success), 0);
}
void ftp_online(int FTP_SEND)
{
    cout<<"FTP_SEND"<<"套接字"<<endl;
    int FTP_RECV=1-FTP_SEND;
    char ftpreq[] = "ftpreq";
    send(conn_fd[FTP_RECV], ftpreq, (int)strlen(ftpreq), 0);
    char ftpstart[] = "ftpstart";
    char ftp_buffer[MAX_BUFF_LEN];
    while(1)
    {
        recv(conn_fd[FTP_SEND], ftp_buffer, MAX_BUFF_LEN, 0);
        if(strncmp(ftp_buffer, "ftpstart", 8) == 0)
            break;
    }
    int test;
    test=send(conn_fd[FTP_RECV], ftpstart, (int)strlen(ftpstart), 0);
    cout<<test<<"开始"<<endl;
    while(1)
    {
        test=recv(conn_fd[FTP_SEND], ftp_buffer, MAX_BUFF_LEN, 0);
        cout<<"收"<<test<<endl;
        if(strncmp(ftp_buffer, "ftpfin", 6) == 0)
            break;
        test=send(conn_fd[FTP_RECV], ftp_buffer, test, 0);
        cout<<"发"<<test<<endl;
    }
    char success[] = "File transfer success!";
    send(conn_fd[FTP_SEND], success, (int)strlen(success), 0);
    char fin[] = "ftpfin";
    sleep(5*100);
    send(conn_fd[FTP_RECV], fin, (int)strlen(fin), 0);
    cout << "File transfer success!"<<endl;
}
void *recv_func(void *arg)
{
    INFO *info = (INFO *)arg;
    pthread_t send_thread0,send_thread1;
    char recv_buffer[MAX_BUFF_LEN];
    int send_result;
    INFO_SEND info_send;

    ID_verify(conn_fd[info->NO]);

    while (1)
    {

        if (recv(info->sock_fd, recv_buffer, sizeof(recv_buffer), 0) > 0)
        {
            cout << "MESSAGE RECV FROM NO." << info->NO << ": " << recv_buffer<<endl;

            if(strncmp(recv_buffer, "ftpreqlixian", 12) == 0)
            {
                cout<<"recereq"<<endl;
                ftp_offline(info->sock_fd);
                continue;
            }
            if(strncmp(recv_buffer, "ftpreqzaixian", 13) == 0)
            {
                ftp_online(info->NO);
                continue;
            }
            if (strncmp(recv_buffer, "quit", 4) == 0)
                break;
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