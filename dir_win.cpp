#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <io.h>
#include <vector>
using namespace std;

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

int main(void)
{
    char current_address[100];
    memset(current_address, 0, 100);
    getcwd(current_address, 100); //获取当前路径
    cout << current_address << endl;
    strcat(current_address, "\\*");

    vector<string> files = getFiles((string)current_address);

    for (int i = 0; i < files.size(); i++)
    {
        char filename[100];
        strcpy(filename, files[i].c_str());
        cout << filename << endl;
    }
    return 0;
}