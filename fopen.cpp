#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

#define MAX_BUFF_LEN 1024    //最大缓冲区长度

using namespace std;
int main()
{
    char path[MAX_BUFF_LEN] = "./Downloads/";
    strcat(path, "pic.jpg");
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
    return 0;
}
