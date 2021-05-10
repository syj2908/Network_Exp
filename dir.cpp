#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <iostream>
using namespace std;

int main(int argc, char * argv[])
{
    struct dirent *ptr;    
    DIR *dir;
    dir=opendir("./");

    int cur = 0;

    char filename[20] = "pic.png";
    strcat(filename, ".tmp");

    while((ptr=readdir(dir))!=NULL)
    {
 
        //跳过'.'和'..'两个目录
        if(ptr->d_name[0] == '.')
            continue;
        // printf("%s\n",ptr->d_name);
        if(strcmp(ptr->d_name, filename) == 0)
        {
            FILE *pFile;
            char filepath[] = "./";
            strcat(filepath, ptr->d_name);
            pFile = fopen(filepath, "rb");
            fseek(pFile, 0, SEEK_END);
            cur = ftell(pFile);
            break;
        }
    }
    cout << "cur: " << cur << endl;
    closedir(dir);
    return 0;
}
