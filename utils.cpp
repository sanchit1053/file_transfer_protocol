#include "utils.h"


FileInfo::FileInfo(string tfileName, int tuniqeID, int tdepth){
    fileName = tfileName;
    uniqueID = tuniqeID;
    depth = tdepth;
}

NeighbourInfo::NeighbourInfo(int tuniqeID, int tdepth, int tport, vector<string> tfiles){
    uniqueID = tuniqeID;
    depth = tdepth;
    port = tport;
    files = tfiles;
}



void printConnections(vector<pair<string,int>> con){
    vector<vector<int>> a;
    for(auto i : con){
        int id, uid;
        breakMessage(i.first, &id, &uid);
        vector<int> x = {id, uid, i.second};
        a.push_back(x);
    }
    sort(a.begin(), a.end());
    for(auto i : a){
        cout << "Connected to " + to_string(i[0]) + " with unique-ID " + to_string(i[1]) + " on port " + to_string(i[2]);
    }
}

string makeInfoMessage(vector<NeighbourInfo> f){
    string ans = "";
    for(auto i : f){
        ans += to_string(i.uniqueID) + "," + to_string(i.depth) + ',' + to_string(i.port) + ',';
        for(auto j : i.files){
            ans += j + ',';
        }
        ans += '|';
    }
    return ans;
}

void addNeighbourInfo(string s, vector<NeighbourInfo>& n){
    stringstream ss1(s);
    string temp;
    while(getline(ss1, temp, '|')){
        stringstream ss2(temp);
        string temp;
        getline(ss2, temp, ',');
        int uni = stoi(temp);
        getline(ss2, temp, ',');
        int depth = stoi(temp);
        getline(ss2, temp, ',');
        int port = stoi(temp);
        vector<string> fw;
        while(getline(ss2, temp, ',')){
            fw.push_back(temp);
        }
        n.push_back(NeighbourInfo(uni, depth, port, fw));
    }
}


vector<string> breakFilesMessage(string s){
    vector<string> ans;
    stringstream ss(s);
    string temp;
    while(getline(ss, temp, ',')){
        ans.push_back(temp);
    }
    return ans;
}


string showNeighbourUniqeId(string buff,  int port){
    int id, uniqueID;
    breakMessage(buff, &id, &uniqueID);
    return "Connected to " + to_string(id) + " with unique-ID " + to_string(uniqueID) + " on port " + to_string(port); 
}

void sigchld_handler(int s){
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

bool allNeighboursFound(vector<bool> found){
    bool allfound = true;
    for(auto f : found ){
        if(!f) allfound = false;
    }
    if(allfound) return true ;
    return false;
}


string fileName(string path){
    return path.substr(path.find_last_of('/') + 1);
}

string getFileName(string s, string dir){
    int a = dir.length();
    string s1 = (s.substr(s.find(dir)+a)) ;
    return s1;
}

void breakMessage(string message, int* id, int* uniqueID){
    *id  = stoi(message.substr(0, message.find(",")));
    *uniqueID =  stoi(message.substr(message.find(",")+1));
}


void addUniqueId(string message, vector<int>& Nid, vector<int>& NuniqueID, vector<bool>& found){
    int id;
    int uniqueID;
    breakMessage(message, &id, &uniqueID);
    for(int i = 0; i < Nid.size() ; i++){
        if(Nid[i] == id){
            NuniqueID[i] = uniqueID;
            found[i] = true;
            break;
        }
    }
}


void showFileLocations(vector<pair<string, int>> fileWanted, string directory, bool md){
    for(auto file : fileWanted){
        int a = file.second;
        bool found = !( a == HUGE_VAL );
        unsigned char c[MD5_DIGEST_LENGTH] = "0";

        // cout << "BROOOO " << MD5_DIGEST_LENGTH << endl;


        if(md){
            if(a == HUGE_VAL){
                a = 0;
            }
            else {
                string path = directory + "Downloaded/" +  file.first;
                FILE* fp = fopen(path.c_str(), "rb");
                if(fp == NULL){
                    // cout << "FILE COULD NOT BE OPENED" << endl;
                    continue;
                }

                MD5_CTX md;
                int bytes;
                int chunk = 2048;
                unsigned char data[chunk];

                MD5_Init (&md);
                while ((bytes = fread (data, 1, chunk, fp)) != 0){
                    // cout << bytes << endl;
                    // cout << "hello" << endl;    
                    MD5_Update (&md, data, bytes);
                }
                MD5_Final (c,&md);  
            }
        }
    
        string md5 = "";
        for(int i = 0; i < MD5_DIGEST_LENGTH; i++){
            md5 += "0123456789ABCDEF"[c[i]/16];
            md5 += "0123456789ABCDEF"[c[i]%16];
            // cout << c[i];
        }
        


        if(!found) md5 = "0";

        if(file.second != 0){
            cout << "found " << file.first << " at " << a <<  " with MD5 " << md5 << " at depth " << found << endl;
        }
        else{
            cout << "found " << file.first << " at " << "0" <<  " with MD5 " << md5 << " at depth " << found <<  endl;
        }
    }
}

void showFileLocations(vector<FileInfo> fileWanted, string directory, bool md){
    for(auto file : fileWanted){
        int a = file.uniqueID;
        bool found = !( a == HUGE_VAL );
        unsigned char c[MD5_DIGEST_LENGTH] = "0";

        // cout << "BROOOO " << MD5_DIGEST_LENGTH << endl;

        if(md){
            if(a == HUGE_VAL){
                // cout << "REACHED";
                a = 0;
            }
            else {


                string path = directory + "Downloaded/" +  file.fileName;
                FILE* fp = fopen(path.c_str(), "rb");
                if(fp == NULL){
                    cout << "FILE COULD NOT BE OPENED" << path << endl;
                    continue;
        // cout << "HELLO" <<endl;
                }

                MD5_CTX md;
                int bytes;
                int chunk = 2048;
                unsigned char data[chunk];

                MD5_Init (&md);
                while ((bytes = fread (data, 1, chunk, fp)) != 0){
                    // cout << bytes << endl;
                    // cout << "hello" << endl;    
                    MD5_Update (&md, data, bytes);
                }
                MD5_Final (c,&md);  
            }
        }
        string md5 = "";
        for(int i = 0; i < MD5_DIGEST_LENGTH; i++){
            md5 += "0123456789ABCDEF"[c[i]/16];
            md5 += "0123456789ABCDEF"[c[i]%16];
            // cout << c[i];
        }
        

        // cout << "HELLO";
        if(!found || !md) md5 = "0";


        if(file.uniqueID != 0){
            cout << "found " << file.fileName << " at " << a <<  " with MD5 " << md5 << " at depth " << file.depth << endl;
        }
        else{
            cout << "found " << file.fileName << " at " << "0" <<  " with MD5 " << md5 << " at depth " << file.depth <<  endl;
        }

        // cout << "ENDING\n";
    }
}

vector<string> breakDelimmiter(string s, char c){
    stringstream ss(s);
    string it;
    vector<string> ans;
    while(getline(ss, it, c)){
        ans.push_back(it);
    }
    return ans;
}


void updateFileWanted(string s, vector<pair<string,int>> &fileWanted, int uniqueID){
    for (int i = 0 ; i < s.length(); i++){
        if(s[i] == '1'){
            if(fileWanted[i].second == 0 || fileWanted[i].second > uniqueID){
                fileWanted[i].second = uniqueID;
            }
        }
    }
}


void updateFileWanted(string s, vector<FileInfo> &fileWanted, int uniqueID){
    for (int i = 0 ; i < s.length(); i++){
        if(s[i] != '0'){
            if(fileWanted[i].uniqueID == 0 || fileWanted[i].uniqueID > uniqueID){
                    fileWanted[i].uniqueID = uniqueID;
                    fileWanted[i].depth = (s[i]) - '0';
                }
            else if(fileWanted[i].uniqueID == uniqueID ){
                if(fileWanted[i].depth == 0 || fileWanted[i].depth < s[i]){
                    fileWanted[i].depth == (s[i]) - '0';
                }
            }
         }
    }
}

void updateFileWanted2(string s, vector<FileInfo>& fileWanted){
    stringstream ss1(s);
    string temp;
    while(getline(ss1, temp, '|')){
        stringstream ss2(temp);
        string temp;
        getline(ss2, temp, ',');
        int uni = stoi(temp);
        getline(ss2, temp, ',');
        int depth = stoi(temp);
        getline(ss2, temp, ',');
        int port = stoi(temp);
        vector<string> fw;
        while(getline(ss2, temp, ',')){
            fw.push_back(temp);
        }
        for(auto& fi : fileWanted){
            for(auto x : fw){
                if(x == fi.fileName){
                    // cout << "Hello  " <<  x << " " <<  uni << " " <<  depth <<" " <<  port << endl;
                    if(fi.depth == 0){
                        // cout << "FIRST\n";
                        fi.depth = depth + 1;
                        fi.uniqueID = uni;
                    }
                    else if(fi.depth == 1){
                        // cout << "SECOND\n ";
                    }
                    else{
                        // cout << "THIRD\n";
                        if(fi.uniqueID > uni){
                            // cout << "FOURTH \n";
                            fi.depth = depth + 1;
                            fi.uniqueID = uni;
                        }
                    }
                    // cout << endl;
                }
            }
        }
    }
}



void recieveFile(int sockfd, string file){

    // cout << "[---] RECIEVING FILE" <<  file <<endl;


    FILE *fp = fopen(file.c_str(), "wb");

    int MAX_DATA_SIZE = 1000000;
    char buff[MAX_DATA_SIZE];

    int numbytes;
    numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0);
    if (numbytes == -1) {
        close(sockfd);
    }
    buff[numbytes] = '\0';
    int filesize = atoi(buff);

    // cout << "[---] RECIEVED FILE SIZE " << filesize << endl;

    string message = "Sanchit was here";
    if (send(sockfd, message.c_str(), message.size(), 0) == -1){
        perror("send");
    }

    // cout << "[---] Send ACK" << endl;

    int bytesRecieved = 0;

    // cout << "REACHED" << endl;

    while(1){


        while(1){
            numbytes = recv(sockfd, buff, MAX_DATA_SIZE, 0);
            // cout << "[---] recieve buffer  " << numbytes << endl;
            
            // buff[numbytes] = '\0';
            // cout << buff << endl;
            
            fwrite(buff, numbytes , 1, fp);

            bytesRecieved += numbytes;
            
            if(bytesRecieved >= filesize){
                break;
            }

            if (numbytes == -1) {
                close(sockfd);
                continue;
            }
            

            // cout << "RE" << endl;

        
        }
        char endMessage[] = "Sanchit";

        // cout << "XXX" << endl;

        if(strcmp(buff, endMessage) == 0){
            cout << "ENDING AFTER SANCHIT" << endl;
            break;
        }

        // cout << "AC" << endl;

        // cout << sizeof(buff);
        // cout << "  " << numbytes << endl;

        // cout << "HE" << endl;

        if (send(sockfd, message.c_str(), message.size(), 0) == -1){
            perror("send");
        }

        // cout << "[---] Send useless " << message << endl;


        if(bytesRecieved >= filesize){
            // cout << "found all words" << endl;
            break;
        }


    }
    numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0);

    // cout << "exited loop" << endl;
    fclose(fp);
    // cout << "will exit func" << endl;

}

void recieveFile2(int sockfd, string file){

    // cout << "[---] RECIEVING FILE" <<  file <<endl;


    FILE *fp = fopen(file.c_str(), "wb");

    int MAX_DATA_SIZE = 1000000;
    char buff[MAX_DATA_SIZE];

    int numbytes;
    numbytes = recv(sockfd, buff, 100, 0);
    if (numbytes == -1) {
        close(sockfd);
    }
    buff[numbytes] = '\0';
    int filesize = atoi(buff);

    // cout << "[---] RECIEVED FILE SIZE " << filesize << endl;

    string message = "Sanchit was here";
    // if (send(sockfd, message.c_str(), message.size(), 0) == -1){
    //     perror("send");
    // }

    // cout << "[---] Send ACK" << endl;

    int bytesRecieved = 0;

    // cout << "REACHED" << endl;

    while(1){


        while(1){
            numbytes = recv(sockfd, buff, filesize, 0);
            // cout << "[---] recieve buffer  " << numbytes << endl;
            
            buff[numbytes] = '\0';
            // cout << buff << endl;
            
            fwrite(buff, numbytes , 1, fp);

            bytesRecieved += numbytes;
            
            if(bytesRecieved >= filesize){
                break;
            }

            if (numbytes == -1) {
                close(sockfd);
                continue;
            }
            

            // cout << "RE" << endl;

        
        }
        char endMessage[] = "Sanchit";

        // cout << "XXX" << endl;

        if(strcmp(buff, endMessage) == 0){
            cout << "ENDING AFTER SANCHIT" << endl;
            break;
        }

        // cout << "AC" << endl;

        // cout << sizeof(buff);
        // cout << "  " << numbytes << endl;

        // cout << "HE" << endl;

        // if (send(sockfd, message.c_str(), message.size(), 0) == -1){
        //     perror("send");
        // }

        // cout << "[---] Send useless " << message << endl;


        if(bytesRecieved >= filesize){
            // cout << "found all words" << endl;
            break;
        }


    }
    numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0);

    // cout << "exited loop" << endl;
    fclose(fp);
    // cout << "will exit func" << endl;

}


void sendFile(int sockfd, string file){

    // cout << "[---] SENDING FILE " << file << endl;

    FILE *fp = fopen(file.c_str(), "rb");

    int MAX_DATA_SIZE = 100;
    char buff[MAX_DATA_SIZE];

    string message = "Sanchit was here";
    
    // cout << "Reached" << endl;

    string fileContents;

    struct stat b{};
    // cout << stat(file.c_str(), &b)<< endl;
    stat(file.c_str(), &b);
    fileContents.resize(b.st_size + 1u);
    fread(const_cast<char*>(fileContents.data()),b.st_size, 1, fp);
    // cout << b.st_size << endl;
    // int size = b.st_size;
    // cout << "---------" << fileContents.length() << endl;

    fclose(fp);

    // const char* charContents 
    // memcpy(charContents, fileContents)= fileContents.c_str();
    int bytesSent = 0;
    int bytesLeft = fileContents.length();

    // cout << strlen(charContents) << endl;
    int fileSize = bytesLeft;
    string filesize = to_string(bytesLeft);


    // cout << fileSize << " " << filesize << " " << bytesLeft << endl;

    if (send(sockfd, filesize.c_str(), filesize.size(), 0) == -1){
        perror("send");
    }

    // cout << "[---] SENT FILE SIZE"  << filesize.c_str() << endl;

    int numbytes;
    
    numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0);
    if (numbytes == -1) {
        close(sockfd);
    }
    buff[numbytes] = '\0';

    // cout << "[---] recieved ACK" << endl;


    // cout << filesize << endl;


    while(bytesSent < fileSize){
        // int len = fread(buff, MAX_DATA_SIZE, 1, fp);


        // cout << "READ " << fileContents.substr(bytesSent).c_str() << endl;
        // int numbytes = send(sockfd, charContents + bytesSent, bytesLeft , 0);
        int numbytes = send(sockfd, fileContents.substr(bytesSent).c_str(), bytesLeft , 0);
        
        
        // cout << "[---] SENT BUFFER " <<  " " << numbytes << endl; 
        
        
        // cout << buff << endl;
        
        
        if(numbytes == -1){
            perror("send");
        }
        bytesSent += numbytes;
        bytesLeft -= numbytes;


        numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0);
        if (numbytes == -1) {
            close(sockfd);
            continue;
        }
        buff[numbytes] = '\0' ;
        // cout << "[---] useless recieve " << buff << endl;

    }

    string sanchit = "Sanchit";
    // cout << "SENDING SANCHIT" << endl;
    if(send(sockfd, sanchit.c_str(), sanchit.size(), 0) == -1){
        perror("send");
    }

    // cout << "[---] sent final message " << endl;
} 


void sendFile2(int sockfd, string file){

    // cout << "[---] SENDING FILE " << file << endl;

    FILE *fp = fopen(file.c_str(), "rb");

    int MAX_DATA_SIZE = 100;
    char buff[MAX_DATA_SIZE];

    string message = "Sanchit was here";
    
    // cout << "Reached" << endl;

    string fileContents;

    struct stat b{};
    // cout << stat(file.c_str(), &b)<< endl;
    stat(file.c_str(), &b);
    fileContents.resize(b.st_size + 1u);
    fread(const_cast<char*>(fileContents.data()),b.st_size, 1, fp);
    // cout << b.st_size << endl;
    // int size = b.st_size;
    // cout << "---------" << fileContents.length() << endl;

    fclose(fp);

    // const char* charContents 
    // memcpy(charContents, fileContents)= fileContents.c_str();
    int bytesSent = 0;
    int bytesLeft = fileContents.length();

    // cout << strlen(charContents) << endl;
    int fileSize = bytesLeft;
    string filesize = to_string(bytesLeft) + "|";


    // cout << fileSize << " " << filesize << " " << bytesLeft << endl;

    if (send(sockfd, filesize.c_str(), 100, 0) == -1){
        perror("send");
    }

    // cout << "[---] SENT FILE SIZE"  << filesize.c_str() << endl;

    int numbytes;
    
    // numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0);
    // if (numbytes == -1) {
    //     close(sockfd);
    // }
    // buff[numbytes] = '\0';

    // cout << "[---] recieved ACK" << endl;


    // cout << filesize << endl;


    while(bytesSent < fileSize){
        // int len = fread(buff, MAX_DATA_SIZE, 1, fp);


        // cout << "READ " << fileContents.substr(bytesSent).c_str() << endl;
        // int numbytes = send(sockfd, charContents + bytesSent, bytesLeft , 0);
        int numbytes = send(sockfd, fileContents.substr(bytesSent).c_str(), bytesLeft , 0);
        
        
        // cout << "[---] SENT BUFFER " <<  " " << numbytes << endl; 
        
        
        // cout << buff << endl;
        
        
        if(numbytes == -1){
            perror("send");
        }
        bytesSent += numbytes;
        bytesLeft -= numbytes;


        // numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0);
        // if (numbytes == -1) {
        //     close(sockfd);
        //     continue;
        // }
        // buff[numbytes] = '\0' ;
        // cout << "[---] useless recieve " << buff << endl;

    }

    string sanchit = "Sanchit";
    // cout << "SENDING SANCHIT" << endl;
    if(send(sockfd, sanchit.c_str(), sanchit.size(), 0) == -1){
        perror("send");
    }

    // cout << "[---] sent final message " << endl;
} 