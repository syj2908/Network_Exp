#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <iostream>
#include <string.h>
using namespace std;

// ���ӵ�����ļ�
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma comment (lib, "pthreadVC2.lib")


#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT "8000"

void log_in (SOCKET* soc)
{
    system("cls");
	cout<<"------------------------------------------------------------------------"<<endl;
    char username[20]={0};
	char secret[10]={0}; 
    char kongge[2]=" ";
    char rec_log[50]={0};
    char c;
log: 
    printf("�����������û�����");
    c=getchar();
	gets_s(username,19);
    printf("�������������룺");
    gets_s(secret,9);
    strcat_s(username, kongge);
    strcat_s(username, secret);
    send(*soc, username, (int)strlen(username), 0 );
    recv(*soc, rec_log, 50, 0);
    puts(rec_log);
    if(rec_log[6]!='s')
    {
        goto log;
    }

}

void person_chat(SOCKET* soc);
void ftp_send(SOCKET* soc)
{
    char ftp_req[50];
    cout << "��ѡ���ļ����䷽ʽ��" << endl;
    cout << "���ߴ���������1" << endl;
    cout << "���ߴ���������2" << endl;
    switch (getchar())
    {

        case('1'):
            strcpy_s(ftp_req, "ftpreqlixian"); 
            send(*soc, ftp_req, (int)strlen(ftp_req), 0);
            break;
        case('2'):
            strcpy_s(ftp_req, "ftpreqzaixian"); //
            send(*soc, ftp_req, (int)strlen(ftp_req), 0);
            break;
        default:
            cout << " ";
    }
    cout << "�����ļ�λ�ã�" << endl;
    cout << "�����ļ�����" << endl;
    char const *filename = "test.png";  //�ļ���
    FILE* fp;
    int err;
    err = fopen_s(&fp, "D:\\my_code\\vs code\\sock1.1\\Debug\\test1.mp4", "rb");
    if (err == 0) 
    {
        printf("The file  was opened\n");
    }
    else 
    {
        printf("The file  was not opened\n");
    }

    int nCount;
    char sendbuf[DEFAULT_BUFLEN] = {0};
    char ftpstart[] = "finstart";
    send(*soc, ftpstart, (int)strlen(ftpstart), 0);
    int test;
    while( (nCount = fread(sendbuf, 1, DEFAULT_BUFLEN, fp)) > 0 )
    {
        test=send(*soc, sendbuf, nCount, 0);

        printf("%d", nCount);
        //if (test == 694)
            //cout << "good";
    }
    fclose(fp);
    char fin[] = "ftpfin";
    send(*soc, fin, (int)strlen(fin), 0);
    printf("�������");

}

void ftp_recv(SOCKET* soc)
{
    char filename[100] = "test.mp4";  //�ļ���
    FILE* fp;
    fopen_s(&fp,filename, "wb"); //�Զ����Ʒ�ʽ�򿪣��������ļ�
    char buffer[DEFAULT_BUFLEN] = { 0 };  //�ļ�������
    int nCount;
    while (1)
    {
        recv(*soc, buffer, DEFAULT_BUFLEN, 0);
        if (strncmp(buffer, "finstart", 8) == 0)
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
    char success[] = "File transfer success!";
    puts("File transfer success!");
    send(*soc, success, sizeof(success), 0);
}
void client_interface(SOCKET* soc)//�ͻ�������
{
    cout<<"------------------------------------------------------------------------"<<endl;
	cout<<"�𾴵Ŀͻ������ã�"<<endl;
	cout<<endl;
	cout<<"��ѡ������Ҫִ�еĲ���:"<<endl;
	cout<<endl;
	cout<<"��������           ������1"<<endl;
	cout<<endl;
	cout<<"��������           ������2"<<endl;
	cout<<endl;
	cout<<"�ļ�����           ������3"<<endl;
	cout<<endl;
    cout<<"����ͨ��           ������4"<<endl;
	cout<<endl;
	cout<<"�˳�����           ������5"<<endl;
	cout<<"------------------------------------------------------------------------"<<endl;
/*loop_ui:
    switch(getchar())
	{
		case('1'):
			system("cls");
            person_chat(soc);
            break;
		case('2'):
			system("cls");
            break;
		case('3'):
            system("cls");
            ftp_send(soc);
            break;
		case('4'):
            break;
        case('5'):
            break;
		default:
			system("cls");
			cout<<"------------------------------------------------------------------------"<<endl;
			cout<<"������Ч ��������ȷ������"<<endl;
			cout<<"------------------------------------------------------------------------"<<endl;
			goto loop_ui;
            //system("pause");

	 } */
}

void *send_func(void *arg)
{
    // Send an initial buffer
    //int iResult;
    char sendbuf[DEFAULT_BUFLEN];
    SOCKET send_sock = (SOCKET)arg ;
    
    /*gets(sendbuf);
    iResult = send(send_sock, sendbuf, (int)strlen(sendbuf), 0 );
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        //closesocket(send_sock);
        //WSACleanup();
        //return 1;
    }*/
    //printf("Bytes Sent: %ld\n", iResult);

    while (1)
    {
        if (fgets(sendbuf, DEFAULT_BUFLEN, stdin) == NULL)
        {
            continue;
        }
        else
        {
            if (strncmp(sendbuf, "quit", 4) == 0)
                break;
            if (strncmp(sendbuf, "ftp", 3) == 0)
            {
                ftp_send(&send_sock);
                continue;
            }
            if (send(send_sock, sendbuf, (int)strlen(sendbuf), 0 ) == -1)
            {
                printf("send failed with error: %d\n", WSAGetLastError());
                continue;
            }
            else
            {
                printf("Bytes Sent: %d\n", (int)strlen(sendbuf));
            }

            /*if (strncmp(sendbuf, "quit", 4) == 0)
                break;*/
        }
    }
    // shutdown the connection since no more data will be sent
    /*iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }*/
    pthread_exit(0);
    return 0;
}
void *recv_func(void *arg)
{
    // Receive until the peer closes the connection
    int iResult;
    SOCKET rece_sock = (SOCKET)arg ;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    
    do {

        iResult = recv(rece_sock, recvbuf, recvbuflen, 0);
        if ( iResult > 0 )
        {
            printf("Bytes received: %d\n", iResult);
            puts(recvbuf);
        }    
        else if ( iResult == 0 )
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while( iResult > 0 );

    return 0;
}
void person_chat(SOCKET* soc)
{
    pthread_t recv_p;
	pthread_t send_p;

    int send_result,recv_result;

    send_result=pthread_create(&send_p, NULL, send_func, (void*)*soc);
    recv_result=pthread_create(&recv_p, NULL, recv_func, (void*)*soc);

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
}
int __cdecl main(int argc, char **argv) 
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;  //����һ��sockaddr���͵Ľṹ��

    int iResult;

    // Validate the parameters
    if (argc != 2) {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }

    // ��ʼ�� Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
        //��������Ws2_32.dll  2,2��ʾ����winsock2.2�汾
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) ); //��0�����hints���ڴ�
    hints.ai_family = AF_UNSPEC; //0����ʾδָ����ipv4����IPV6
    hints.ai_socktype = SOCK_STREAM;  //1,��ʾscoket����tcp
    hints.ai_protocol = IPPROTO_TCP;    //6����ʾ���ݴ�������

    // Resolve the server address and port
    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result); //8000
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) 
    {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    system("cls");
    cout<<"------------------------------------------------------------------------"<<endl;
	cout<<"��ӭ�����׺�������!!!"<<endl;
	cout<<endl;
	cout<<"��ѡ������Ҫִ�еĲ���:"<<endl;
	cout<<endl;
	cout<<"��¼�����ʺ�  ������1"<<endl;
	cout<<endl;
	cout<<"ע���˺�      ������2"<<endl;
	cout<<endl;
	cout<<"                                                              �汾v1.0.0"<<endl;
	cout<<"                                                ��Ȩ���У�XJTU �ߺ��� ʩ�׽� "<<endl;
	cout<<"------------------------------------------------------------------------"<<endl;

    LOOP:switch(getchar())
	{
		case('1'):
			log_in(&ConnectSocket); 
			break;
		case('2'):
			//id_register();
			break;
		default:
			system("cls");
			cout<<"------------------------------------------------------------------------"<<endl;
			cout<<"������Ч ��������ȷ������"<<endl;
			cout<<"------------------------------------------------------------------------"<<endl;
			system("pause");
            goto LOOP;
	} 

    //��ʼ�շ��ļ�
    pthread_t recv_p;
    pthread_t send_p;

    int send_result, recv_result;

    send_result = pthread_create(&send_p, NULL, send_func, (void*)ConnectSocket);
    recv_result = pthread_create(&recv_p, NULL, recv_func, (void*)ConnectSocket);

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

    client_interface(&ConnectSocket);

    send_result = pthread_join(send_p, NULL);
    recv_result = pthread_join(recv_p, NULL);

    printf("SYSTEM: Chat over.\n");

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}