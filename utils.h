#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>
#include <openssl/md5.h>
#include<sstream>

using namespace std;

#define HUGE_VAL 2147483627


class FileInfo{
    public:
    string fileName;
    int uniqueID;
    int depth;
    
    FileInfo(string tfileName, int tuniqeID, int tdepth);
};

class NeighbourInfo{
    public:
    int uniqueID;
    int depth;
    int port;
    vector<string> files;

    NeighbourInfo(int tuniqeID, int tdepth, int tport, vector<string> tfiles);
};

string showNeighbourUniqeId(string buff, int port);

void sigchld_handler(int s);

void *get_in_addr(struct sockaddr *sa);

string fileName(string path);

bool allNeighboursFound(vector<bool> found);

string getFileName(string s, string dir);

void breakMessage(string message, int* id, int* uniqueID);
vector<string> breakFilesMessage(string s);

string makeInfoMessage(vector<NeighbourInfo> f);

void addUniqueId(string message, vector<int>& Nid, vector<int>& NuniqueID, vector<bool>& found);

void addNeighbourInfo(string s, vector<NeighbourInfo>& n);

void showFileLocations(vector<pair<string, int>> fileWanted, string dir, bool md);

void showFileLocations(vector<FileInfo> fileWanted, string dir, bool md);

vector<string> breakDelimmiter(string s, char c);

void updateFileWanted(string s, vector<pair<string,int>>& fileWanted, int uniqueID);

void updateFileWanted(string s, vector<FileInfo>& fileWanted, int uniqueID);

void updateFileWanted2(string s, vector<FileInfo>& fileWanted);

void recieveFile(int sockfd, string file);
void recieveFile2(int sockfd, string file);

void sendFile(int sockfd, string file);
void sendFile2(int sockfd, string file);

void printConnections(vector<pair<string,int>> con);


#endif 

