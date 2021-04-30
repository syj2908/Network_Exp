//192.168.43.219
#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT "8000"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <iostream>
#include <string.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")
#pragma comment(lib, "pthreadVC2.lib")

using namespace std;

void login(SOCKET *soc);
void ftp_send(SOCKET *soc);
void ftp_recv(SOCKET *soc);
void *send_func(void *arg);
void *recv_func(void *arg);
void main_UI(SOCKET *soc);

void login(SOCKET *soc)
{
    //check username and passwd

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

void ftp_send(SOCKET *soc)
{
    char ftp_req[50];

    cout << "1.Offline" << endl;
    cout << "2.Online" << endl;

    switch (getchar())
    {
        case ('1'):
            strcpy(ftp_req, "ftpreqlixian");
            send(*soc, ftp_req, (int)strlen(ftp_req), 0);
            break;
        case ('2'):
            strcpy(ftp_req, "ftpreqzaixian");
            send(*soc, ftp_req, (int)strlen(ftp_req), 0);
            break;
    }
    cout << "Location: " << endl;

    //char const *filename = "test"; //文件名

    FILE *fp = fopen("D:\\Algorithm\\Network\\ChatRoom\\text.txt", "rb");

    if (fp==NULL)
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
    char ftpstart[] = "ftpstart";
    send(*soc, ftpstart, (int)strlen(ftpstart), 0);
    int test;
    while ((nCount = fread(sendbuf, 1, DEFAULT_BUFLEN, fp)) > 0)
    {
        //sendbuf[nCount] = '\0';
        test = send(*soc, sendbuf, nCount, 0);
        printf("%d\n", test);
    }
    fclose(fp);
    char fin[] = "ftpfin";
    Sleep(50);
    test = send(*soc, fin, (int)strlen(fin), 0);
    printf("%d", test);
    printf("Done.");
}

void ftp_recv(SOCKET *soc)
{
    char filename[100] = "recv.txt"; //文件名

    FILE *fp = fopen(filename, "wb");  //以二进制方式打开（创建）文件
    char buffer[DEFAULT_BUFLEN] = {0}; //文件缓冲区
    int nCount;
    while (1)
    {
        recv(*soc, buffer, DEFAULT_BUFLEN, 0);
        if (strncmp(buffer, "ftpstart", 8) == 0)
            break;
    }
    cout << "start";
    while (1)
    {
        nCount = recv(*soc, buffer, DEFAULT_BUFLEN, 0);
        if (strncmp(buffer, "ftpfin", 6) == 0)
            break;
        if (nCount > 0)
        {
            fwrite(buffer, nCount, 1, fp);
        }
    }
    fclose(fp);
    //char success[] = "File transfer success!";
    cout << "File transfer success!";
    //send(*soc, success, sizeof(success), 0);
}

void *send_func(void *arg)
{
    //send thread's function

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
                break;
            if (strncmp(sendbuf, "FTP", 3) == 0)
            {
                ftp_send(&send_sock);
                continue;
            }
            if (send(send_sock, sendbuf, (int)strlen(sendbuf), 0) == -1)
            {
                printf("-->Send failed with error: %d\n", WSAGetLastError());
                continue;
            }
        }
    }

    pthread_exit(0);
}

void *recv_func(void *arg)
{
    //recv thread's function

    SOCKET rece_sock = (SOCKET)arg;
    int iResult;
    char recvbuf[DEFAULT_BUFLEN] = {0};
    int recvbuflen = DEFAULT_BUFLEN;

    do
    {
        iResult = recv(rece_sock, recvbuf, recvbuflen, 0);

        if (strncmp(recvbuf, "ftpreq", 6) == 0)
        {
            ftp_recv(&rece_sock);
            continue;
        }

        if (iResult > 0)
        {
            cout << "RECV MESSAGE: " << recvbuf;
        }
        else if (iResult == 0)
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while (iResult > 0);

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

int __cdecl main(int argc, char **argv)
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    int iResult;

    if (argc != 2)
    {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (iResult != 0)
    {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints)); //用0来填充hints的内存
    hints.ai_family = AF_UNSPEC;       //0，表示未指明是ipv4还是IPV6
    hints.ai_socktype = SOCK_STREAM;   //1,表示scoket类型tcp
    hints.ai_protocol = IPPROTO_TCP;   //6，表示数据传输类型

    // Resolve the server address and port
    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result); //8000
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
    {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET)
        {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR)
        {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET)
    {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    main_UI(&ConnectSocket);

    pthread_t recv_p;
    pthread_t send_p;

    int send_result, recv_result;

    send_result = pthread_create(&send_p, NULL, send_func, (void *)ConnectSocket);
    recv_result = pthread_create(&recv_p, NULL, recv_func, (void *)ConnectSocket);

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
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}