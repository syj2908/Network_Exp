#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT 8000
#define FTP_PORT 8006
#define IP "192.168.140.128"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <iostream>
#include <string.h>
#include <direct.h>
#include <io.h>
#include <vector>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")
#pragma comment(lib, "pthreadVC2.lib")

using namespace std;

long cur;

struct CMD
{
    //1:offline 2:online
    int method;
    //0:sender 1:receiver
    int sender;
    //filename
    char filename[20];
    //cur
    long cur;
};

void login(SOCKET *soc);
void ftp_send(SOCKET *soc, CMD cmd);
void ftp_recv(SOCKET *soc, CMD cmd);
void *send_func(void *arg);
void *recv_func(void *arg);
void main_UI(SOCKET *soc);

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

vector<string> getFiles(string cate_dir)
{
    vector<string> files; //存放文件名
    _finddata_t file;
    long lf = _findfirst(cate_dir.c_str(), &file);

    if (lf == -1)
    {
        cout << cate_dir << " not found!!!" << endl;
    }
    else
    {
        while (_findnext(lf, &file) == 0)
        {
            if (strcmp(file.name, ".") == 0 || strcmp(file.name, "..") == 0)
                continue;
            files.push_back(file.name);
        }
    }
    _findclose(lf);
    return files;
}

void login(SOCKET *soc)
{
    system("cls");

    char username[20] = {0};
    char passwd[20] = {0};
    char uname_pwd[50] = {0};
    char space[] = " ";
    char rec_log[10] = {0};

    cout << "------------------------------------------------------------------------" << endl;

    while (1)
    {
        printf("Please input your UserName: ");
        cin.sync();
        gets(username);
        printf("Please input your PassWord: ");
        cin.sync();
        gets(passwd);

        strcat(username, space);
        strcat(username, passwd);
        strcpy(uname_pwd, username);

        send(*soc, uname_pwd, (int)strlen(uname_pwd), 0);
        recv(*soc, rec_log, 10, 0);

        if (strncmp(rec_log, "1", 1) == 0)
        {
            break;
        }
    }
    printf("Authentication succeeded, linked to ChatRoom.\n");
}

void ftp_send(SOCKET *soc, CMD cmd)
{
    Sleep(50);

    char fname[30] = ".\\";

    strcat(fname, cmd.filename);
    FILE *fp = fopen(fname, "rb");

    if (fp == NULL)
    {
        printf("ERROR: The file was not opened.\n");
        return;
    }
    else
    {
        printf("Opening file...\n");
    }

    int nCount;
    char sendbuf[DEFAULT_BUFLEN] = {0};
    int send_ret;
    int sum = 0;

    if (cmd.method == 1)
    {
        fpos_t fcur;
        fcur = cmd.cur;
        fsetpos(fp, &fcur);
    }

    Sleep(50);

    while ((nCount = fread(sendbuf, 1, DEFAULT_BUFLEN, fp)) > 0)
    {
        send_ret = send(*soc, sendbuf, nCount, 0);
        Sleep(5);
        sum += send_ret;
        cout << "send signal:" << send_ret << endl;
        if (nCount < DEFAULT_BUFLEN)
            break;
    }

    cout << "sum bytes send: " << sum << endl;
    fclose(fp);

    char fin[] = "FTPfin";
    Sleep(50);
    send_ret = send(*soc, fin, (int)strlen(fin), 0);
    cout << "send signal:" << send_ret << endl;

    cout << "Done." << endl;
}

void ftp_recv(SOCKET *soc, CMD cmd)
{
    cout << "ftp recv" << endl;
    cout << "cmd.filename: " << cmd.filename << endl;
    FILE *fp = fopen(cmd.filename, "ab"); //以二进制方式打开（创建）文件
    char buffer[DEFAULT_BUFLEN] = {0};    //文件缓冲区
    int nCount;
    int sum = 0;

    while (1)
    {
        nCount = recv(*soc, buffer, DEFAULT_BUFLEN, 0);
        cout << "recv signal: " << nCount << endl;
        if (strncmp(buffer, "FTPfin", 6) == 0)
            break;
        if (nCount > 0)
        {
            fwrite(buffer, nCount, 1, fp);
        }
        sum += nCount;
    }

    cout << "sum bytes received: " << sum << endl;
    fclose(fp);

    char newname[DEFAULT_BUFLEN] = {0};
    int len = strlen(cmd.filename);
    strncat(newname, cmd.filename, len - 4);
    cout << "cmd.filename=" << cmd.filename << endl;
    cout << "newname=" << newname << endl;
    int re_ret = rename(cmd.filename, newname);
    if (re_ret != 0)
    {
        cout << "re_ret=" << re_ret << endl;
        cout << errno << endl;
    }
    cout << "File transfer success!" << endl;
}

void *ftp_func(void *arg)
{
    CMD cmd = *(CMD *)arg;
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA data;

    if (WSAStartup(sockVersion, &data) != 0)
    {
        pthread_exit(0);
    }

    SOCKET ftpsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (ftpsock == INVALID_SOCKET)
    {
        cout << ("invalid socket!");
        pthread_exit(0);
    }

    sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(FTP_PORT);
    serAddr.sin_addr.S_un.S_addr = inet_addr(IP);

    cout << "Connecting to ftp service." << endl;

    if (connect(ftpsock, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
    {
        cout << "ftp_func Connect error." << endl;
        closesocket(ftpsock);
        pthread_exit(0);
    }

    if (cmd.method == 1)
    {
        //offline
        if (cmd.sender == 0)
        {
            ftp_send(&ftpsock, cmd);
        }
        else if (cmd.sender == 1)
        {
            ftp_recv(&ftpsock, cmd);
        }
    }
    else if (cmd.method == 2)
    {
        //online

        if (cmd.sender == 0)
        {
            char signal[] = "I am sender.";
            send(ftpsock, signal, (int)strlen(signal), 0);
            cout << "ID send." << endl;
            ftp_send(&ftpsock, cmd);
        }
        else if (cmd.sender == 1)
        {
            char signal[] = "I am receiver.";
            send(ftpsock, signal, (int)strlen(signal), 0);
            cout << "ID send." << endl;
            ftp_recv(&ftpsock, cmd);
        }
    }

    closesocket(ftpsock);
    WSACleanup();
    pthread_exit(0);
}

void *send_func(void *arg)
{
    char sendbuf[DEFAULT_BUFLEN];
    SOCKET send_sock = (SOCKET)arg;

    while (1)
    {
        if (fgets(sendbuf, DEFAULT_BUFLEN, stdin) == NULL)
        {
            continue;
        }
        else
        {
            if (strncmp(sendbuf, "QUIT", 4) == 0)
            {
                break;
            }
            else if (strncmp(sendbuf, "FTP", 3) == 0)
            {
                CMD cmd;
                cmd.sender = 0;
                char ftpreq[50];
                strcpy(ftpreq, "FTP");
                send(send_sock, ftpreq, (int)strlen(ftpreq), 0);

                cout << "1.Offline" << endl;
                cout << "2.Online" << endl;

                switch (getchar())
                {
                case ('1'):
                {
                    cmd.method = 1;
                    strcpy(ftpreq, "offline");
                    send(send_sock, ftpreq, (int)strlen(ftpreq), 0);
                    Sleep(50);

                    char pos[50] = {0};
                    cout << "Location: " << endl;
                    cin >> pos;
                    strcpy(cmd.filename, pos);

                    if (send(send_sock, pos, (int)strlen(pos), 0) == -1)
                    {
                        cout << "send error." << endl;
                    }
                    cout << "send filename " << pos << endl;

                    Sleep(50);
                    cmd.cur = cur;
                    cout << "send_func cur: " << cmd.cur << endl;
                    break;
                }
                case ('2'):
                {
                    cmd.method = 2;
                    strcpy(ftpreq, "online");
                    send(send_sock, ftpreq, (int)strlen(ftpreq), 0);
                    break;
                }
                }

                pthread_t ftp_thread;
                int ftp_result;
                ftp_result = pthread_create(&ftp_thread, NULL, ftp_func, (void *)&cmd);
                ftp_result = pthread_join(ftp_thread, NULL);
                cout << "Back to main thread." << endl;
                continue;
            }
            if (send(send_sock, sendbuf, (int)strlen(sendbuf), 0) == -1)
            {
                printf("Send failed with error: %d\n", WSAGetLastError());
                continue;
            }
            if (strncmp(sendbuf, "GET ", 4) == 0)
            {
                CMD cmd;
                cmd.method = 1;
                cmd.sender = 1;
                char filename[DEFAULT_BUFLEN];
                format(sendbuf, sizeof(sendbuf));
                strcpy(filename, sendbuf + 4);
                strcat(filename, ".tmp");
                strcpy(cmd.filename, filename);
                char all_filename[DEFAULT_BUFLEN];

                char current_address[DEFAULT_BUFLEN];
                memset(current_address, 0, DEFAULT_BUFLEN);
                getcwd(current_address, DEFAULT_BUFLEN);
                strcat(current_address, "\\*");

                vector<string> files = getFiles((string)current_address);

                for (int i = 0; i < files.size(); i++)
                {
                    strcpy(all_filename, files[i].c_str());
                    if (strcmp(filename, all_filename) == 0)
                    {
                        //temp file exist
                        FILE *pFile;
                        char filepath[] = "./";
                        strcat(filepath, filename);
                        pFile = fopen(filepath, "rb");
                        fseek(pFile, 0, SEEK_END);
                        cmd.cur = ftell(pFile);
                        break;
                    }
                }

                cout << "cur: " << cmd.cur << endl;
                char chcur[10] = {0};
                sprintf(chcur, "%ld", cmd.cur);
                cout << "send cur: " << chcur << endl;
                char sendbuffer[20] = "cur:";
                strcat(sendbuffer, chcur);
                Sleep(50);
                send(send_sock, sendbuffer, (int)strlen(sendbuffer), 0);

                pthread_t ftp_thread;
                int ftp_result;
                ftp_result = pthread_create(&ftp_thread, NULL, ftp_func, (void *)&cmd);
                ftp_result = pthread_join(ftp_thread, NULL);
                continue;
            }
        }
    }
    pthread_exit(0);
}

void *recv_func(void *arg)
{
    SOCKET recv_sock = (SOCKET)arg;
    int recv_ret;
    char recvbuf[DEFAULT_BUFLEN] = {0};
    int recvbuflen = DEFAULT_BUFLEN;

    do
    {
        recv_ret = recv(recv_sock, recvbuf, recvbuflen, 0);

        if (strncmp(recvbuf, "online", 6) == 0)
        {
            pthread_t ftp_thread;
            int ftp_result;
            CMD cmd;
            cmd.method = 2;
            cmd.sender = 1;
            ftp_result = pthread_create(&ftp_thread, NULL, ftp_func, (void *)&cmd);
            ftp_result = pthread_join(ftp_thread, NULL);
            continue;
        }
        else if (strncmp(recvbuf, "cur:", 4) == 0)
        {
            char chcur[10] = {0};
            strncpy(chcur, recvbuf + 4, 10);
            cur = atol(chcur);
        }
        else if (recv_ret > 0)
        {
            cout << "RECV MESSAGE: " << recvbuf;
        }
        else if (recv_ret == 0)
            printf("Connection closed.\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while (recv_ret > 0);

    pthread_exit(0);
}

void main_UI(SOCKET *soc)
{
    //pirnt main UI

    system("cls");
    cout << "------------------------------------------------------------------------" << endl;
    cout << "Input the operation: " << endl;
    cout << endl;
    cout << "1. Sign in." << endl;
    cout << endl;
    cout << "2. Sign up." << endl;
    cout << "------------------------------------------------------------------------" << endl;

    while (1)
    {
        switch (getchar())
        {
        case ('1'):
            login(soc);
            break;
        case ('2'):
            break;
        default:
            system("cls");
            cout << "------------------------------------------------------------------------" << endl;
            cout << "Undefined opreation." << endl;
            cout << "------------------------------------------------------------------------" << endl;
            system("pause");
        }
        break;
    }
}

int main()
{
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA data;
    if (WSAStartup(sockVersion, &data) != 0)
    {
        return 0;
    }

    SOCKET sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sclient == INVALID_SOCKET)
    {
        cout << ("invalid socket!");
        return 0;
    }

    sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(DEFAULT_PORT);
    serAddr.sin_addr.S_un.S_addr = inet_addr(IP);
    if (connect(sclient, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
    {
        cout << "Main Connect error." << endl;
        closesocket(sclient);
        return 0;
    }

    main_UI(&sclient);

    pthread_t recv_p, send_p;
    int send_result, recv_result;

    send_result = pthread_create(&send_p, NULL, send_func, (void *)sclient);
    recv_result = pthread_create(&recv_p, NULL, recv_func, (void *)sclient);

    if (send_result != 0)
    {
        printf("send_thread create fail\n");
        exit(1);
    }
    if (recv_result != 0)
    {
        printf("recv_thread create fail\n");
        exit(1);
    }

    send_result = pthread_join(send_p, NULL);
    recv_result = pthread_join(recv_p, NULL);

    printf("SYSTEM: Chat over.\n");

    // cleanup
    closesocket(sclient);
    WSACleanup();
    return 0;
}
