#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT 8000
#define FTP_PORT 8006
#define VOICE_PORT 8088
#define DATASIZE 800 //分次截取数据大小
#define IP "172.16.250.40"

//#define IP "124.71.224.149"

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
#include <mmeapi.h>
#include <conio.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")
#pragma comment(lib, "pthreadVC2.lib")
#pragma comment(lib, "winmm.lib")
using namespace std;

SOCKET sock;

HWAVEIN hWaveIn;        //输入设备
tWAVEFORMATEX waveform; //采集音频的格式，结构体
char *pBuffer1;
char *pBuffer2; //采集音频时的数据缓存
char *pBuffer3;
WAVEHDR wHdr1, wHdr2; //采集音频时包含数据缓存的结构体
//FILE* pf;

int bufsize = 800;
//FILE* pcmfile;  //音频文件
HWAVEOUT hwo;
int nAudioOut;
int nReceive;
long cur;

struct CAudioOutData //队列结构，用来存储网络中接收到的音频数据
{
    char *lpdata;
    int dwLength;
};
struct CAudioOutData m_AudioDataOut[50];

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
void sign_up(SOCKET *soc);
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
    char signin[] = "1";
    send(*soc, signin, (int)strlen(signin), 0);
    char username[20] = {0};
    char passwd[20] = {0};
    char uname_pwd[50] = {0};
    char space[] = " ";
    char rec_log[10] = {0};

    cout << "------------------------------------------------------------------------" << endl;

    while (1)
    {
        while (_kbhit())
        {
            getch();
        }
        rewind(stdin);
        printf("Please input your UserName: ");
        cin.clear();
        cin.sync();
        gets_s(username);
        printf("Please input your PassWord: ");
        cin.clear();
        cin.sync();
        gets_s(passwd);

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

void sign_up(SOCKET *soc)
{
    system("cls");

    char signup[] = "2";
    send(*soc, signup, (int)strlen(signup), 0);
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
        gets_s(username);
        printf("Please input your PassWord: ");
        cin.sync();
        gets_s(passwd);

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
    printf("The account registration is successful. Please log in again.\n");
    Sleep(3000);
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
    FILE *fp = fopen(cmd.filename, "wb"); //以二进制方式打开（创建）文件
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
    return 0;
}

void CALLBACK waveInProc(HWAVEIN hWave, UINT uMsg, DWORD dwInstance, DWORD dw1, DWORD dw2) //回调函数
{

    switch (uMsg)
    {
    case WIM_DATA: //缓冲录满或停止录音消息
    {
        LPWAVEHDR pWaveHeader = (LPWAVEHDR)dw1;
        int result;
        //fwrite(pWaveHeader->lpData, 1, pWaveHeader->dwBytesRecorded, pf);
        result = send(sock, pWaveHeader->lpData, pWaveHeader->dwBytesRecorded, 0);
        cout << "send:" << result;
        waveInPrepareHeader(hWaveIn, pWaveHeader, sizeof(WAVEHDR));
        waveInAddBuffer(hWaveIn, pWaveHeader, sizeof(WAVEHDR));
        break;
    }
    case WIM_CLOSE: //音频输入设备关闭消息
    {
        waveInUnprepareHeader(hWaveIn, &wHdr1, sizeof(WAVEHDR));
        waveInUnprepareHeader(hWaveIn, &wHdr2, sizeof(WAVEHDR));
    }
    }
}

void CALLBACK WaveCallback(HWAVEOUT hWave, UINT uMsg, DWORD dwInstance, DWORD dw1, DWORD dw2) //回调函数
{
    switch (uMsg)
    {
    case WOM_DONE: //上次缓存播放完成,触发该事件
    {
        LPWAVEHDR pWaveHeader = (LPWAVEHDR)dw1;
        pWaveHeader->dwBufferLength = DATASIZE;
        memcpy(
            pWaveHeader->lpData,
            m_AudioDataOut[nAudioOut].lpdata,
            m_AudioDataOut[nAudioOut].dwLength);
        //fwrite(m_AudioDataOut[nAudioOut].lpdata, 1, m_AudioDataOut[nAudioOut].dwLength, pcmfile);
        waveOutPrepareHeader(hwo, pWaveHeader, sizeof(WAVEHDR));
        waveOutWrite(hwo, pWaveHeader, sizeof(WAVEHDR));
        nAudioOut++;

        if (nAudioOut == 50)
        {
            nAudioOut = 0;
        }
        cout << nAudioOut << nReceive << endl;
    }
        //case WOM_CLOSE:
        //{
        //    LPWAVEHDR pWaveHeader = (LPWAVEHDR)dw1;
        //    waveOutUnprepareHeader(hwo, pWaveHeader, sizeof(WAVEHDR));
        //    free(pWaveHeader);
        //    pWaveHeader = NULL;
        //}
    }
}

void *voice_send_func(void *arg)
{

    sock = (SOCKET)arg;

    waveform.wFormatTag = WAVE_FORMAT_PCM; //声音格式为PCM
    waveform.nSamplesPerSec = 8000;        //采样率，16000次/秒
    waveform.wBitsPerSample = 16;          //采样比特，16bits/次
    waveform.nChannels = 1;                //采样声道数，2声道
    waveform.nAvgBytesPerSec = 16000;      //每秒的数据率，就是每秒能采集多少字节的数据
    waveform.nBlockAlign = 2;              //一个块的大小，采样bit的字节数乘以声道数
    waveform.cbSize = 0;                   //一般为0

    waveInOpen(&hWaveIn, WAVE_MAPPER, &waveform, (DWORD_PTR)waveInProc, 0L, CALLBACK_FUNCTION);

    //建立两个数组（这里可以建立多个数组）用来缓冲音频数据
    //每次开辟10k的缓存存储录音数据
    //fopen_s(&pf, "test.pcm", "wb");

    pBuffer1 = new char[bufsize];
    pBuffer2 = new char[bufsize];
    wHdr1.lpData = (LPSTR)pBuffer1;
    wHdr1.dwBufferLength = bufsize;
    wHdr1.dwBytesRecorded = 0;
    wHdr1.dwUser = 0;
    wHdr1.dwFlags = 0;
    wHdr1.dwLoops = 1;
    wHdr1.lpNext = NULL;
    waveInPrepareHeader(hWaveIn, &wHdr1, sizeof(WAVEHDR)); //准备一个波形数据块头用于录音

    wHdr2.lpData = (LPSTR)pBuffer2;
    wHdr2.dwBufferLength = bufsize;
    wHdr2.dwBytesRecorded = 0;
    wHdr2.dwUser = 0;
    wHdr2.dwFlags = 0;
    wHdr2.dwLoops = 1;
    wHdr2.lpNext = NULL;
    waveInPrepareHeader(hWaveIn, &wHdr2, sizeof(WAVEHDR)); //准备一个波形数据块头用于录音

    waveInAddBuffer(hWaveIn, &wHdr1, sizeof(WAVEHDR)); //指定波形数据块为录音输入缓存
    waveInAddBuffer(hWaveIn, &wHdr2, sizeof(WAVEHDR));

    waveInStart(hWaveIn); //开始录音
    cout << "test" << endl;
    //waveInReset(hWaveIn);//停止录音
    while (1)
    {
        if (_kbhit() && _getch() == 0x1b)
        {
            cout << "The phone has been cut off." << endl;
            break;
        }
    }
    delete pBuffer1;
    delete pBuffer2;

    waveInClose(hWaveIn);

    pthread_exit(0);
    return 0;
}

void *voice_recv_func(void *arg)
{
    for (int i = 0; i <= 49; i++)
    {
        m_AudioDataOut[i].lpdata = new char[DATASIZE];
    }
    nReceive = 0;
    SOCKET rece_sock = (SOCKET)arg;

    do
    {

        m_AudioDataOut[nReceive].dwLength = recv(rece_sock, m_AudioDataOut[nReceive].lpdata, DATASIZE, 0);
        if (m_AudioDataOut[nReceive].dwLength > 0)
        {
            printf("Bytes received: %d\n", m_AudioDataOut[nReceive].dwLength);
            nReceive++;
            if (nReceive == 50)
            {
                nReceive = 0;
            }
        }
        else if (m_AudioDataOut[nReceive].dwLength == 0)
        {
            printf("Connection closed\n");
            break;
        }
        else
        {
            printf("recv failed with error: %d\n", WSAGetLastError());
            break;
        }
    } while (1);

    pthread_exit(0);
    return 0;
}

void *vout(void *arg)
{

    WAVEFORMATEX wfx;
    WAVEHDR pWaveHdrOut[8];
    //fopen_s(&pcmfile, "test1.pcm", "wb");//打开文件

    wfx.wFormatTag = WAVE_FORMAT_PCM; //设置波形声音的格式
    wfx.nChannels = 1;                //设置音频文件的通道数量
    wfx.nSamplesPerSec = 8000;        //设置每个声道播放和记录时的样本频率
    wfx.nAvgBytesPerSec = 16000;      //设置请求的平均数据传输率,单位byte/s。这个值对于创建缓冲大小是很有用的
    wfx.nBlockAlign = 2;              //以字节为单位设置块对齐
    wfx.wBitsPerSample = 16;
    wfx.cbSize = 0; //额外信息的大小

    waveOutOpen(&hwo, WAVE_MAPPER, &wfx, (DWORD)WaveCallback, NULL, CALLBACK_FUNCTION); //打开一个给定的波形音频输出装置来进行声音播放

    int BufferNum = 8;
    char *outBuffer[8];
    for (int i = 0; i < BufferNum; i++)
    {
        outBuffer[i] = new char[DATASIZE];
    }
    for (int i = 0; i < BufferNum; i++) // BufferNum 为输出缓冲块数
    {                                   // outBuffer[i]是每一块缓冲区的 首地址，为short类型
        pWaveHdrOut[i].lpData = (LPSTR)outBuffer[i];
        pWaveHdrOut[i].dwBufferLength = DATASIZE;
        pWaveHdrOut[i].dwBytesRecorded = 0;
        pWaveHdrOut[i].dwUser = 0;
        pWaveHdrOut[i].dwFlags = 0;
        pWaveHdrOut[i].dwLoops = 1;
        pWaveHdrOut[i].lpNext = NULL;
        pWaveHdrOut[i].reserved = 0;
    }

    nAudioOut = 0;

    while (1)
    {                     // nReceive 是网络音频接收指针在循环队列中的位置
        if (nReceive > 2) // 当接收到的数据大于1帧时才启动声卡输出
        {
            for (int i = 0; i < BufferNum; i++)
            { // 启动音频输出所有缓冲区块
                waveOutPrepareHeader(hwo, &pWaveHdrOut[i], sizeof(WAVEHDR));
                waveOutWrite(hwo, &pWaveHdrOut[i], sizeof(WAVEHDR));
            }
            break;
        }
        Sleep(100);
    }
    while (1)
    {
        if (_kbhit() && _getch() == 0x1b)
        {
            //cout << "The phone has been cut off." << endl;
            break;
        }
    }
    pthread_exit(0);
    return 0;
}

void *voice_func(void *arg)
{
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA data;

    if (WSAStartup(sockVersion, &data) != 0)
    {
        pthread_exit(0);
    }

    SOCKET voicesock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (voicesock == INVALID_SOCKET)
    {
        cout << ("invalid socket!");
        pthread_exit(0);
    }

    sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(VOICE_PORT);
    serAddr.sin_addr.S_un.S_addr = inet_addr(IP);

    cout << "Connecting to voice call service." << endl;

    if (connect(voicesock, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
    {
        cout << "voice_func Connect error." << endl;
        closesocket(voicesock);
        pthread_exit(0);
    }

    pthread_t recv_p;
    pthread_t send_p;
    pthread_t voice_out;

    int send_result, recv_result, vout_result;

    send_result = pthread_create(&send_p, NULL, voice_send_func, (void *)voicesock);
    recv_result = pthread_create(&recv_p, NULL, voice_recv_func, (void *)voicesock);
    vout_result = pthread_create(&voice_out, NULL, vout, (void *)voicesock);

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
    vout_result = pthread_join(voice_out, NULL);
    closesocket(voicesock);
    WSACleanup();
    pthread_exit(0);
    return 0;
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
            else if (strncmp(sendbuf, "VOICE", 5) == 0)
            {
                char voicereq[10];
                strcpy(voicereq, "VOICE");
                send(send_sock, voicereq, (int)strlen(voicereq), 0);
                pthread_t voice_thread;
                int voice_result;
                int a = 0;
                voice_result = pthread_create(&voice_thread, NULL, voice_func, (void *)&a);
                voice_result = pthread_join(voice_thread, NULL);
                cout << "Back to main thread." << endl;
                continue;
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
    return 0;
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
        else if (strncmp(recvbuf, "VOICE", 5) == 0)
        {
            cout << "Voice call start." << endl;
            pthread_t voice_thread;
            int voice_result;
            voice_result = pthread_create(&voice_thread, NULL, voice_func, NULL);
            voice_result = pthread_join(voice_thread, NULL);
            continue;
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
    return 0;
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
            sign_up(soc);
            login(soc);
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
