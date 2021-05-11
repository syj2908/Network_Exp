#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <iostream>
using namespace std;

int main(int argc, char * argv[])
{
    struct dirent *ptr;    
    DIR *dir;
    dir=opendir("./Downloads");

    while((ptr=readdir(dir))!=NULL)
    {
 
        //跳过'.'和'..'两个目录
        if(ptr->d_name[0] == '.')
            continue;
        printf("%s\n",ptr->d_name);
    }
    closedir(dir);
    return 0;
}
