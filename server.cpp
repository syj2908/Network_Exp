#define IP "172.16.250.40"
#define PORT 8000 //服务器端口号
#define ftp_PORT 8006
#define VOICE_PORT 8088   //文件传输端口号
#define QUE_NUM 2         //最大连接数
#define MAX_BUFF_LEN 1024 //最大缓冲区长度
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h> //g++ -pthread -o server server.cpp
#include <string.h>
#include <string>
#include <stdio.h>
#include <dirent.h>
#include <fstream>

int conn_fd[QUE_NUM]; //所有用户的套接字
int voice_fd[QUE_NUM];
int ftp_fd[QUE_NUM]; //文件传输用的套接字
int sender;          //文件发送方info->NO

using namespace std;

long cur;

void *recv_func(void *arg);

struct INFO
{
    //传递给常驻recv线程的所有参数
    int sock_fd; //套接字
    int NO;      //编号
};

struct INFO_SEND
{
    //传递给send线程的所有参数
    int num_send;
    int dst_sock_fd;           //目的用户套接字
    int NO;                    //编号
    char buffer[MAX_BUFF_LEN]; //待发送内容
};

struct CMD
{
    //0:offline_send 1:offline_recv 2:online
    int method;
    char filename[MAX_BUFF_LEN + 4];
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

int ID_verify(int sock_fd)
{
    char signkind[5] = {0};
    recv(sock_fd, signkind, sizeof(signkind), 0);
    char signin[] = "1";
    char signup[] = "2";
    if (strcmp(signkind, signin) == 0)
    {
        ifstream inFile("password.txt", ios::in);
        if (!inFile)
        { //打开失败
            cout << "error opening source file." << endl;
            return 0;
        }
        char Key[100] = {0};
        string passwd;
        char success[] = "1";
        char fail[] = "-1";
        int if_sign = 0;
        while (1)
        {
            if (recv(sock_fd, Key, sizeof(Key), 0) > 0)
            {
                inFile.clear();
                inFile.seekg(0, std::ios::beg);
                while (!inFile.eof())
                {
                    getline(inFile, passwd);
                    cout << passwd << endl;
                    if (strcmp(Key, passwd.c_str()) == 0)
                    {
                        if_sign = 1;
                        cout << "User " << Key << " has login." << endl;
                        send(sock_fd, success, (int)strlen(success), 0);
                        break;
                    }
                }
                if (if_sign == 1)
                    break;
                format(Key, sizeof(Key));
                //memset(passwd,'\0',sizeof(passwd));
                cout << "User " << Key << " is trying to login." << endl;
                send(sock_fd, fail, (int)strlen(fail), 0);
            }
        }
        inFile.close();
        return 1;
    }
    else if (strcmp(signkind, signup) == 0)
    {
        ofstream outFile("password.txt", ios::app); //以文本模式打开out.txt备写
        outFile.seekp(0, ios::end);
        char Key[100] = {0};
        char success[] = "1";
        recv(sock_fd, Key, sizeof(Key), 0);
        outFile << Key << endl;
        outFile.close();
        send(sock_fd, success, (int)strlen(success), 0);
        return 0;
    }
}

void ftp_offline_recv(int sock_fd, CMD cmd)
{
    char path[MAX_BUFF_LEN] = "./Downloads/";
    strcat(path, cmd.filename);
    format(path, sizeof(path));
    FILE *fp = fopen(path, "ab");    //以二进制方式打开（创建）文件，指针自动定位到文件末尾
    char buffer[MAX_BUFF_LEN] = {0}; //文件缓冲区
    int nCount;
    int sum = 0;

    while (1)
    {
        nCount = recv(sock_fd, buffer, MAX_BUFF_LEN, 0);
        cout << "recv signal: " << nCount << endl;
        if (strncmp(buffer, "FTPfin", 6) == 0)
            break;
        if (nCount > 0)
        {
            fwrite(buffer, nCount, 1, fp);
        }
        else
            break;
        sum += nCount;
    }
    cout << "sum bytes received: " << sum << endl;
    fclose(fp);

    char newname[MAX_BUFF_LEN] = "./Downloads/";
    int len = strlen(cmd.filename);
    strncat(newname, cmd.filename, len - 4);
    rename(cmd.filename, newname);
}

void ftp_offline_send(int sock_fd, CMD cmd)
{
    sleep(1);
    cout << "cmd.filename= " << cmd.filename << endl;
    char path[MAX_BUFF_LEN] = "./Downloads/";
    strcat(path, cmd.filename);
    format(path, sizeof(path));
    cout << "full name= " << path << endl;
    FILE *fp = fopen(path, "rb");

    if (fp == NULL)
    {
        printf("ERROR: The file was not opened.\n");
        cout << "errno: " << errno << endl;
    }
    else
    {
        printf("Opening file...\n");
    }

    int nCount;
    char sendbuf[MAX_BUFF_LEN] = {0};
    int send_ret;
    int sum = 0;

    fpos_t fcur;
    fcur.__pos = cur;
    fsetpos(fp, &fcur);

    cout << "cur: " << cur << endl;

    sleep(1);

    while ((nCount = fread(sendbuf, 1, MAX_BUFF_LEN, fp)) > 0)
    {
        send_ret = send(sock_fd, sendbuf, nCount, 0);
        usleep(50);
        sum += send_ret;
        cout << "send signal:" << send_ret << endl;
        if (nCount < MAX_BUFF_LEN)
            break;
    }

    cout << "sum bytes send: " << sum << endl;
    fclose(fp);

    char fin[] = "FTPfin";
    sleep(1);
    send_ret = send(sock_fd, fin, (int)strlen(fin), 0);
    cout << "send signal:" << send_ret << endl;

    cout << "Done." << endl;
}

void ftp_online(int FTP_SEND)
{
    int FTP_RECV = 1 - FTP_SEND;
    int send_ret, recv_ret;
    char ftp_buffer[MAX_BUFF_LEN];

    cout << "FTP online" << endl;

    while (1)
    {
        recv_ret = recv(ftp_fd[FTP_SEND], ftp_buffer, MAX_BUFF_LEN, 0);
        if (recv_ret > 0)
        {
            cout << "recv signal: " << recv_ret << endl;
            if (strncmp(ftp_buffer, "FTPfin", 6) == 0)
            {
                sleep(1);
                char fin[] = "FTPfin";
                send(ftp_fd[FTP_RECV], fin, (int)strlen(fin), 0);
                cout << "send FTPfin." << endl;
                break;
            }
            else
            {
                send_ret = send(ftp_fd[FTP_RECV], ftp_buffer, recv_ret, 0);
                cout << "send signal: " << send_ret << endl;
            }
        }
    }
}

void *waiting_func(void *arg)
{
    INFO *info = (INFO *)arg;
    char recv_buffer[MAX_BUFF_LEN];

    if (recv(info->sock_fd, recv_buffer, sizeof(recv_buffer), 0) > 0)
    {
        if (strncmp(recv_buffer, "I am sender.", 10) == 0)
        {
            sender = info->NO;
        }
    }
    pthread_exit(0);
}

void *ftp_func(void *arg)
{
    CMD cmd = *(CMD *)arg;

    //1:offline 2:online

    int max_link = (cmd.method == 1 || cmd.method == 0) ? 1 : 2;
    int link_num;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in client_addr[max_link];
    INFO info[max_link];
    pthread_t waiting_thread0, waiting_thread1;
    int waiting_result[max_link];

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

    if (listen(sock_fd, max_link) == -1)
    {
        cout << "ftp listen fail" << endl;
        exit(1);
    }

    cout << "preparing for file transmission..." << endl;

    for (link_num = 0; link_num < max_link; link_num++)
    {
        socklen_t client_addr_size = sizeof(client_addr[link_num]);
        if ((ftp_fd[link_num] = accept(sock_fd, (struct sockaddr *)&client_addr[link_num], &client_addr_size)) == -1)
        {
            cout << "ftp accept fail" << endl;
            exit(1);
        }

        cout << link_num << "User " << inet_ntoa(client_addr[link_num].sin_addr) << " is ready to transmit file!" << endl;

        info[link_num].sock_fd = ftp_fd[link_num];
        info[link_num].NO = link_num;
    }

    if (cmd.method == 0)
    {
        cout << "offline_send function." << endl;
        ftp_offline_send(ftp_fd[0], cmd);
    }
    else if (cmd.method == 1)
    {
        cout << "offline_recv function." << endl;
        ftp_offline_recv(ftp_fd[0], cmd);
    }
    else if (cmd.method == 2)
    {
        cout << "waiting for ID..." << endl;
        waiting_result[0] = pthread_create(&waiting_thread0, NULL, waiting_func, &info[0]);
        waiting_result[1] = pthread_create(&waiting_thread1, NULL, waiting_func, &info[1]);

        waiting_result[0] = pthread_join(waiting_thread0, NULL);
        waiting_result[1] = pthread_join(waiting_thread1, NULL);
        cout << "Sender confirm." << endl;

        ftp_online(sender);
    }

    cout << "File transmission success!" << endl;

    if (cmd.method == 1)
    {
        close(ftp_fd[0]);
    }
    else if (cmd.method == 2)
    {
        for (int i = 0; i < 2; i++)
            close(ftp_fd[i]);
    }
    close(sock_fd);
    pthread_exit(0);
}

void *voice_send_func(void *arg)
{
    INFO_SEND *info_send = (INFO_SEND *)arg;

    if (send(info_send->dst_sock_fd, info_send->buffer, info_send->num_send, 0) == -1)
    {
        cout << "send message fail" << endl;
        pthread_exit(0);
    }
    cout << info_send->num_send << endl;

    pthread_exit(0);
}

void *voice_recv_func(void *arg)
{
    INFO *info = (INFO *)arg;
    pthread_t send_thread0, send_thread1;
    char recv_buffer[MAX_BUFF_LEN];
    int send_result;
    INFO_SEND info_send;

    while (1)
    {

        int num = recv(info->sock_fd, recv_buffer, sizeof(recv_buffer), 0);
        if (num > 0)
        {
            cout << "voice recive" << num << endl;
            //if (strncmp(recv_buffer, "quit", 4) == 0)
            //break;
            info_send.num_send = num;
            info_send.NO = info->NO;
            info_send.dst_sock_fd = (info->NO == 0) ? voice_fd[1] : voice_fd[0];
            memcpy(info_send.buffer, recv_buffer, num);

            if (info->NO == 0)
            {
                send_result = pthread_create(&send_thread0, NULL, voice_send_func, &info_send);
            }
            else if (info->NO == 1)
            {
                send_result = pthread_create(&send_thread1, NULL, voice_send_func, &info_send);
            }
        }
        if (num == 0)
        {
            break;
        }
        if (info->NO == 0)
        {
            send_result = pthread_join(send_thread0, NULL);
        }
        else if (info->NO == 1)
        {
            send_result = pthread_join(send_thread1, NULL);
        }
    }
    pthread_exit(0);
}

void *voice_func(void *arg)
{
    pthread_t recv_thread0, recv_thread1;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    int recv_result[QUE_NUM];
    int link_num = 0;                        //已连接人数
    struct sockaddr_in client_addr[QUE_NUM]; //客户端地址数组
    INFO info[QUE_NUM];                      //传递参数的数组

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(VOICE_PORT);
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
        if ((voice_fd[link_num] = accept(sock_fd, (struct sockaddr *)&client_addr[link_num], &client_addr_size)) == -1)
        {
            cout << "accept fail" << endl;
            exit(1);
        }

        cout << "User " << inet_ntoa(client_addr[link_num].sin_addr) << " Has Connect!" << endl;

        info[link_num].sock_fd = voice_fd[link_num];
        info[link_num].NO = link_num;

        if (link_num == 0)
        {
            recv_result[link_num] = pthread_create(&recv_thread0, NULL, voice_recv_func, &info[link_num]);
        }
        else if (link_num == 1)
        {
            recv_result[link_num] = pthread_create(&recv_thread1, NULL, voice_recv_func, &info[link_num]);
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

    cout << "SYSTEM: Voice call over." << endl;

    for (int i = 0; i < 2; i++)
        close(voice_fd[i]);
    close(sock_fd);
    pthread_exit(0);
    return 0;
}

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
        cout << "MESSAGE SEND TO " << (info_send->NO == 0 ? 1 : 0) << ": " << info_send->buffer << endl;
    }

    pthread_exit(0);
}

void *recv_func(void *arg)
{
    INFO *info = (INFO *)arg;
    pthread_t send_thread, ftp_thread;
    char recv_buffer[MAX_BUFF_LEN];
    int send_result;
    INFO_SEND info_send;

    if (ID_verify(conn_fd[info->NO]) == 0)
    {
        ID_verify(conn_fd[info->NO]);
    }

    while (1)
    {
        int recv_num = recv(info->sock_fd, recv_buffer, sizeof(recv_buffer), 0);
        if (recv_num > 0)
        {
            if (strncmp(recv_buffer, "FTP", 3) == 0)
            {
                CMD cmd;
                cout << "Receive FTP request." << endl;

                if (recv(info->sock_fd, recv_buffer, sizeof(recv_buffer), 0) > 0)
                {
                    if (strncmp(recv_buffer, "offline", 7) == 0)
                    {
                        cout << "method: offline." << endl;
                        cmd.method = 1;

                        long cur = 0;

                        if (recv(info->sock_fd, recv_buffer, sizeof(recv_buffer), 0) > 0)
                        {
                            cout << "recv filename is: " << recv_buffer << endl;
                            strcat(recv_buffer, ".tmp");
                            strcpy(cmd.filename, recv_buffer);

                            struct dirent *ptr;
                            DIR *dir;
                            dir = opendir("./Downloads");

                            while ((ptr = readdir(dir)) != NULL)
                            {
                                if (ptr->d_name[0] == '.')
                                {
                                    continue;
                                }
                                if (strcmp(ptr->d_name, recv_buffer) == 0)
                                {
                                    //temp file exist
                                    FILE *pFile;
                                    char filepath[] = "./Downloads/";
                                    //char filepathtem[]="/" ;
                                    //strcat(filepath, );
                                    strcat(filepath, ptr->d_name);
                                    pFile = fopen(filepath, "rb");
                                    fseek(pFile, 0, SEEK_END);
                                    cur = ftell(pFile);
                                    break;
                                }
                            }
                            closedir(dir);

                            cout << "cur: " << cur << endl;
                            char chcur[10] = {0};
                            sprintf(chcur, "%ld", cur);
                            cout << "send cur: " << chcur << endl;
                            char sendbuffer[20] = "cur:";
                            strcat(sendbuffer, chcur);

                            send(info->sock_fd, sendbuffer, (int)strlen(sendbuffer), 0);

                            info_send.NO = info->NO;
                            info_send.dst_sock_fd = (info->NO == 0) ? conn_fd[1] : conn_fd[0];
                            char ftpsignal[] = "You have a file ready to accept, type \"FILELIST\" to figure out!\n";
                            strcpy(info_send.buffer, ftpsignal);

                            send_result = pthread_create(&send_thread, NULL, send_func, &info_send);
                        }
                    }
                    else if (strncmp(recv_buffer, "online", 6) == 0)
                    {
                        cout << "method: online." << endl;
                        cmd.method = 2;
                        info_send.NO = info->NO;
                        info_send.dst_sock_fd = (info->NO == 0) ? conn_fd[1] : conn_fd[0];
                        strcpy(info_send.buffer, recv_buffer);
                        send_result = pthread_create(&send_thread, NULL, send_func, &info_send);
                    }
                    // memset(recv_buffer, '\0', sizeof(recv_buffer));
                }
                int ftp_result = pthread_create(&ftp_thread, NULL, ftp_func, &cmd);
                ftp_result = pthread_join(ftp_thread, NULL);
                continue;
            }
            else if (strncmp(recv_buffer, "FILELIST", 8) == 0)
            {
                struct dirent *ptr;
                DIR *dir;
                dir = opendir("./Downloads");

                info_send.NO = info->NO;
                info_send.dst_sock_fd = (info->NO == 0) ? conn_fd[0] : conn_fd[1];
                char filelist[] = "******File List******\n";
                strcpy(info_send.buffer, filelist);
                send_result = pthread_create(&send_thread, NULL, send_func, &info_send);
                send_result = pthread_join(send_thread, NULL);
                while ((ptr = readdir(dir)) != NULL)
                {
                    //跳过'.'和'..'两个目录
                    if (ptr->d_name[0] == '.')
                        continue;
                    char filename[MAX_BUFF_LEN] = "-->";
                    strcat(filename, ptr->d_name);
                    strcat(filename, "\n");
                    strcpy(info_send.buffer, filename);
                    send_result = pthread_create(&send_thread, NULL, send_func, &info_send);
                    send_result = pthread_join(send_thread, NULL);
                }
                closedir(dir);
            }
            else if (strncmp(recv_buffer, "GET ", 4) == 0)
            {
                CMD cmd;
                cmd.method = 0;
                strcat(cmd.filename, recv_buffer + 4);
                cout << "Receive FTP_GET request, filename= " << cmd.filename << endl;

                if (recv(info->sock_fd, recv_buffer, sizeof(recv_buffer), 0) > 0)
                {
                    char chcur[10] = {0};
                    strncpy(chcur, recv_buffer + 4, 10);
                    cur = atol(chcur);
                    cout << "cur: " << cur << endl;
                    int ftp_result = pthread_create(&ftp_thread, NULL, ftp_func, &cmd);
                    ftp_result = pthread_join(ftp_thread, NULL);
                    cout << "ftp_thread exit." << endl;
                }
            }
            else if (strncmp(recv_buffer, "QUIT", 4) == 0)
            {
                break;
            }
            else if (strncmp(recv_buffer, "VOICE", 5) == 0)
            {
                cout << "Receive VOICE CALL request." << endl;

                info_send.NO = info->NO;
                info_send.dst_sock_fd = (info->NO == 0) ? conn_fd[1] : conn_fd[0];
                memcpy(info_send.buffer, recv_buffer, recv_num);
                send_result = pthread_create(&send_thread, NULL, send_func, &info_send);

                pthread_t voice_p;
                int voice_result = pthread_create(&voice_p, NULL, voice_func, NULL);
                voice_result = pthread_join(voice_p, NULL);
                cout << "Call over.Back to main thread." << endl;
                continue;
            }
            else
            {
                cout << "MESSAGE RECV FROM NO." << info->NO << ": " << recv_buffer << endl;

                //cout<<"RECV:"<<recv_num<<endl;
                info_send.NO = info->NO;
                info_send.dst_sock_fd = (info->NO == 0) ? conn_fd[1] : conn_fd[0];
                memcpy(info_send.buffer, recv_buffer, recv_num);
                send_result = pthread_create(&send_thread, NULL, send_func, &info_send);
            }
        }
    }
    pthread_exit(0);
}

int main()
{
    pthread_t recv_thread0, recv_thread1;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    int recv_result[QUE_NUM];
    int link_num = 0;                        //已连接人数
    struct sockaddr_in client_addr[QUE_NUM]; //客户端地址数组
    INFO info[QUE_NUM];                      //传递参数的数组

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
