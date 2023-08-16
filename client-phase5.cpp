///////////////////////////////////////
// CLIENT PHASE-1
///////////////////////////////////////
// PROTOCOL
// FIRST TRY TO FIND ALREADY LISTENING SOCKET FOR ALL PORTS FOR NEIGHBOURING MACHINES
// IF FOUND EXCHANGE DATA
// IF NOT THEN SET UP A LISTENING SOCKET ON YOUR OWN PORT AND WAIT FOR OTHERS TO SEARCH FOR YOU
///////////////////////////////////////
// DATA SENT IS "ID, UNIQUE-ID"
// THE RECIEVER BREAKS IT DOWN TO GET THE INFORMATION
///////////////////////////////////////


//   g++ -std=c++17 client-phase3.cpp utils.cpp -o client-phase3 -lcrypto -lssl


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>
#include <poll.h>

#include <filesystem>
#include <sys/stat.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm> 
#include <sstream> 
#include "utils.h"
#include <openssl/md5.h>   

using namespace std;
namespace fs = std::filesystem;

#define MAX_DATA_SIZE 1000
#define BACKLOG 10
#define HUGE_VAL 2147483627


int main(int argc, char *argv[])
{
    
    
    if (argc != 3) {
        fprintf(stderr,"Usage: (executable argument1-config-file argument2-directory-path)\n");
        exit(1);
    }

    ///////////////////////////////////////
    //   READING THE DIRECTORY AND PRINTING ALL THE FILE NAMES EXCEPT DOWNLOADED FOLDER 
    ///////////////////////////////////////

    vector<pair<string,int>> ownedFilesNames;
    vector<pair<string, int>> connections;


    bool DownloadedFolderPresent = false;
    string directory = argv[2];
    vector<string> tempFile;
    for(const auto file : fs :: directory_iterator(directory)){
        if(!fs::is_directory(file.status())){
            if(getFileName(file.path(), directory).find("Downloaded") == string::npos) {
                string FILENAME = getFileName(file.path(), directory);
                tempFile.push_back(fileName(FILENAME));
                // cout << getFileName(file.path(), directory) << "\n";
                ownedFilesNames.push_back({FILENAME, count(FILENAME.begin(), FILENAME.end(),'/') + 1});

            }
            else{
                DownloadedFolderPresent = true;
            }
        }
    }

    sort(tempFile.begin(), tempFile.end());

    string ownedFiles = "";

    for(auto file : tempFile){
        cout << file << "\n";
        ownedFiles += file + ',';
    }


    if(!DownloadedFolderPresent){

        string path = directory + "Downloaded";
        mkdir(path.c_str(), path.length());
    }
    
    ///////////////////////////////////////
    // READING THE CONFIG FILE AND STORING ALL THE INFORMATION IN VARIABLES
    ///////////////////////////////////////
    
    vector<NeighbourInfo> neighbourInfo;

    int id, port, uniqueID;
    int nNeighbours;
    vector<int> Nid, Nport;
    vector<bool> found;
    int nFileWanted;
    vector<FileInfo> fileWanted;
    vector<int> NuniqueID;

    ifstream configFile(argv[1]);
    configFile >> id;
    configFile >> port;
    configFile >> uniqueID;
    configFile >> nNeighbours;

    vector<pair<int, int>> sd(nNeighbours);

    for(int i = 0; i < nNeighbours; i++){
        int temp;
        configFile >> temp;
        Nid.push_back(temp);
        configFile >> temp;
        Nport.push_back(temp);
        found.push_back(false);
        NuniqueID.push_back(-1); 
    }

    configFile >> nFileWanted;
    for(int i = 0; i < nFileWanted; i ++){
        string temp ;
        configFile >> temp;
        fileWanted.push_back({temp, HUGE_VAL, 0});
    }
    configFile.close();

    string message = to_string(id) + "," + to_string(uniqueID) + "," + to_string(port);    
    string messagefiles = "";
    for(auto file : fileWanted){
        messagefiles += file.fileName + ',';
    }

    const char* local = "localhost";
    int cnt = 0;

    ///////////////////////////////////////
    // FOR EACH PORT TRYING TO FIND IF IT IS BEING LISTENED ON AND KEEPING TRACK OF WHAT HAVE BEEN SEEN 
    ///////////////////////////////////////
    
    for(int i = 0; i < nNeighbours; i++){
        int PORT = Nport[i];
        int sockfd;
        char buff[MAX_DATA_SIZE];
        int rv;
        char s[INET6_ADDRSTRLEN];
        struct addrinfo hints, *servinfo, *p;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET; // set to AF_INET to use IPv4
        hints.ai_socktype = SOCK_STREAM;

        rv = getaddrinfo(local, to_string(PORT).c_str(), &hints, &servinfo);
        if (rv != 0) {
            continue;
        }

        for(p = servinfo; p != NULL; p = p->ai_next) {
            sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol);
            if (sockfd == -1) {
                continue;
            }
            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                // perror("client: connect");
                continue;
            }
            break;
        }

        sd[i].second = sockfd;

        if (p == NULL) {
            // fprintf(stderr, "talker: failed to create socket\n");
            close(sockfd);
            continue;
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
        // cout << "connecting to " << s << "\n";

        freeaddrinfo(servinfo);
    
        
        ///////////////////////////////////
        /// getting the unique ID
        ///////////////////////////////////

        int numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0);
        if (numbytes == -1) {
            close(sockfd);
            continue;
        }
        buff[numbytes]  = '\0';
        
        addUniqueId(buff , Nid, NuniqueID, found);
        int tid, tuniqeID;
        breakMessage(buff, &tid, &tuniqeID);
        // cout << showNeighbourUniqeId(buff, PORT) << endl;
        // cout << "[+] first recived " << buff <<  endl;
        connections.push_back({buff, PORT});

        sd[i].first = tuniqeID;

        ///////////////////////////////////
        /// sending the unique ID
        ///////////////////////////////////

        if (send(sockfd, message.c_str(), message.size(), 0) == -1){
            perror("send");
        }

        // cout << "[+] first sent " << message << endl;
        
        //////////////////////////////////////////////////////////////////////////////////
        if ((numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0)) == -1) {
            perror("talker: sendto");
            exit(1);
        }
        buff[numbytes] = '\0';
        vector<string> nFiles = breakFilesMessage(buff);
        // for(auto i : nFiles){
        //     cout << i << endl;
        // }
        neighbourInfo.push_back(NeighbourInfo(tuniqeID, 1, PORT, nFiles));

        if (send(sockfd, ownedFiles.c_str(), ownedFiles.size(), 0) == -1){
            perror("send");
        }
        //////////////////////////////////////////////////////////////////////////////////




        ///////////////////////////////////
        /// recieving The files wanted 
        ///////////////////////////////////

        numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0);
        if (numbytes == -1) {
            close(sockfd);
            continue;
        }
        buff[numbytes]  = '\0';

        // cout << "[+] second recieved " << buff << endl;
        
        string reply = "";
        vector<string> filesWantedByListener = breakDelimmiter(buff, ',');
        for(auto file : filesWantedByListener){
            auto x = find_if(ownedFilesNames.begin(), ownedFilesNames.end(), [file](pair<string, int> const &p){return fileName(p.first) == file;});
            if(x != ownedFilesNames.end()){
                reply += to_string((*x).second);
            }
            else reply += '0';
        }

        ///////////////////////////////////
        /// replying if it has the required files 
        ///////////////////////////////////
 
        if (send(sockfd, reply.c_str(), reply.size(), 0) == -1){
            perror("send");
        }
        // cout << "[+] second sent "  << reply << endl;



        /// Recieving the request for files  
        numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0);
        if (numbytes == -1) {
            close(sockfd);
            continue;
        }
        buff[numbytes]  = '\0';
        // cout << "[/] files request " << buff << endl;


        char requestedFiles[MAX_DATA_SIZE];
        strcpy(requestedFiles, buff);


        // cout << "FILES " << requestedFiles << " " << strlen(requestedFiles) << endl;
        for(int i = 0; i < strlen(requestedFiles); i++){
            // cout << "_________________" << i << strlen(requestedFiles) << endl;
            char r = requestedFiles[i];
            string file = filesWantedByListener[i];
            if(r == '1'){
                sendFile(sockfd, directory + file);

                // cout << "[+] FILE sent " << file << endl;

                /// TO keep order 
                numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0);
                if (numbytes == -1) {
                    close(sockfd);
                    continue;
                }
                buff[numbytes]  = '\0';

                // cout << "[/] useless recieve " << buff <<  endl;
            
            }


        }

        /// TO keep order
        // cout << "HELLO " << endl;
        if (send(sockfd, reply.c_str(), reply.size(), 0) == -1){
            perror("send");
        }

        // cout << "[/] useless send " << reply << endl;



        ///////////////////////////////////
        /// A recv to keep the line of recieve and send same
        ///////////////////////////////////


        numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0);
        if (numbytes == -1) {
            close(sockfd);
            continue;
        }
        buff[numbytes]  = '\0';


        // cout << "[+] third recieved " << buff << endl;

        ///////////////////////////////////
        /// Sending the files it wants
        ///////////////////////////////////

        // cout << " FILES SENT " << messagefiles << endl;
        if (send(sockfd, messagefiles.c_str(), messagefiles.size(), 0) == -1){
            perror("send");
        }

        // cout << "[+] Third sent " << messagefiles << endl;


        ///////////////////////////////////
        /// Recieving a answer to what files the other person has
        ///////////////////////////////////


        if ((numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0)) == -1) {
            perror("talker: sendto");
            exit(1);
        }
        buff[numbytes] = '\0';

        // cout << "[+] fourth Recieved " << buff <<  endl;


        string request = "";
        for(int i = 0 ; i< strlen(buff); i++){
            char b = buff[i];
            if(b == '0'){
                request += "0";
            }
            else{
                // cout << tuniqeID << " CHEK " << fileWanted[i].uniqueID << endl; 
                if(tuniqeID < fileWanted[i].uniqueID){
                    request += b;
                }
                else{
                    // cout << "LINE 349 \n";
                    request += "0";
                }
            }
        }

        updateFileWanted(buff, fileWanted, tuniqeID);

        /// TO keep order  
        if (send(sockfd, request.c_str(), request.size(), 0) == -1){
            perror("send");
        }

        // cout << "[/] Request send " << request << endl;

        // cout << "FILES RECIIEVED " << buff << endl;
        for(int i = 0; i < numbytes; i++){
            char r = request[i];
            string file = fileWanted[i].fileName ;
            if(r != '0'){
                recieveFile(sockfd, directory + "Downloaded/" + file);
            
                // cout << "[+] FILES recieved " << file << endl;

                /// TO keep order 
                if (send(sockfd, reply.c_str(), reply.size(), 0) == -1){
                    perror("send");
                }

                // cout << "[/] useless send " << reply <<  endl;
            
            }
        }
        

        cnt++;
        // close(sockfd);
    }
    
    // if(allNeighboursFound(found)){
    //     showFileLocations(fileWanted, directory, true);
    //     return 0;
    // }


    // cout << "[-] starting to listen" << endl;

    // None of the ports are already estabilished start listening on the ports
    int sockfd1;
    int yes = 1;
    struct sigaction sa;
    struct addrinfo hints1, *servinfo1, *p1;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    char s1[INET6_ADDRSTRLEN];
    char buff1[MAX_DATA_SIZE];
    int new_fd;
    
    int fd_count = 0;
    int fd_size = nNeighbours - cnt + 1 ;
    struct pollfd *pfds = (struct pollfd*) malloc(sizeof *pfds * (fd_size));

    memset(&hints1, 0, sizeof hints1);
    hints1.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints1.ai_socktype = SOCK_STREAM;
    hints1.ai_flags = AI_PASSIVE;

    int rv1 = getaddrinfo(NULL, to_string(port).c_str(), &hints1, &servinfo1);
    if (rv1 != 0) {
        // fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        // cout << "-> getaddrinfo " << gai_strerror(rv1) <<endl;
        return 1;
    }   

    for(p1 = servinfo1; p1 != NULL; p1 = p1->ai_next) {
        sockfd1 = socket(p1->ai_family, p1->ai_socktype,p1->ai_protocol);
        if (sockfd1 == -1) {
            perror("talker: socket");
            continue;
        }

        if (setsockopt(sockfd1, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd1, p1->ai_addr, p1->ai_addrlen) == -1) {
            close(sockfd1);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo1);

    if (p1 == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        exit(1);
        // return 2;
    }

    // sockfd1 is the listener socket for the polling

    if (listen(sockfd1, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    pfds[0].fd =  sockfd1;
    pfds[0].events = POLLIN;

    fd_count = 1;

    int counter = 0;

    vector<bool> alreadyChecked(fd_size, false);

    while(true){        
        if(counter == fd_size - 1){
            break;
        }

        int poll_count = poll( pfds, fd_count, -1);

        if(poll_count == -1){
            perror("poll");
            exit(1);
        }

        for (int i = 0 ; i < fd_count ; i++){
            // cout << fd_count << endl;
            // for(auto i : alreadyChecked) cout << i << " ";
            // cout << endl;
            if(alreadyChecked[i])
                continue;

            
            if(pfds[i].revents & POLLIN){

                if(pfds[i].fd == sockfd1){
                    sin_size = sizeof their_addr;
                    new_fd = accept(sockfd1, (struct sockaddr *) &their_addr, &sin_size);

                    if(new_fd == -1){
                        perror("accept");
                    }
                    else{
                        pfds[fd_count].fd = new_fd;
                        pfds[fd_count].events = POLLIN;
                        fd_count++;
                        
                        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *) &their_addr), s1, sizeof s1);

                        int send_fd = new_fd;

                        if (send(send_fd, message.c_str(), message.size(), 0) == -1){
                            perror("send");
                        }
                        ///////////////////////////////////////////////////////////////////////////
                        // cout << "[*] first sent " << message <<  endl;

                        int numbytes;
                        if ((numbytes = recv(send_fd, buff1, MAX_DATA_SIZE - 1, 0)) == -1) {
                            perror("talker: sendto");
                            exit(1);
                        }
                        buff1[numbytes] = '\0';

                        // cout << "[*] first recieved " << buff1 <<  endl;
                        
                        addUniqueId(buff1, Nid, NuniqueID, found);


                        int tid, tuniqeID;
                        breakMessage(buff1, &tid, &tuniqeID);
                        string b = buff1;
                        int p = stoi(b.substr(b.find_last_of(',') + 1));
                        

                        for(int i= 0; i < sd.size(); i++){
                            if(Nid[i] == tid){
                                sd[i].first =NuniqueID[i];
                                sd[i].second = send_fd;
                            }
                        }

                        // cout << "HELLO " << buff1 << " "  << p  << endl;
                        // cout << showNeighbourUniqeId(buff1, p) << endl;
                        connections.push_back({buff1, p});

                        //////////////////////////////////////////////////////////////////////////////////////////////////
                        if (send(send_fd, ownedFiles.c_str(), ownedFiles.size(), 0) == -1){
                            perror("send");
                        }


                        if ((numbytes = recv(send_fd, buff1, MAX_DATA_SIZE - 1, 0)) == -1) {
                            perror("talker: sendto");
                            exit(1);
                        }
                        buff1[numbytes] = '\0';
                        vector<string> nFiles = breakFilesMessage(buff1);
                        // for(auto i : nFiles){
                        //     cout << i << endl;
                        // }
                        neighbourInfo.push_back(NeighbourInfo(tuniqeID, 1, p, nFiles));
                        //////////////////////////////////////////////////////////////////////////////////////////////////


                        

                        if (send(send_fd, messagefiles.c_str(), messagefiles.size(), 0) == -1){
                            perror("send");
                        }

                        // cout << "[*] second sent " << messagefiles <<  endl;

                        if ((numbytes = recv(send_fd, buff1, MAX_DATA_SIZE - 1, 0)) == -1) {
                            perror("talker: sendto");
                            exit(1);
                        }

                        

                        buff1[numbytes] = '\0';
                        // cout << "[*] second recived " << buff1 <<  endl;
                        
                        
                        string request = "";
                        for(int i = 0; i < strlen(buff1); i++){
                            char b = buff1[i];
                            if(b == '0'){
                                // cout << "B is 0" << endl;
                                request += "0";
                            }
                            else{
                                // cout << tuniqeID << " " << fileWanted[i].second << endl;
                                if(tuniqeID < fileWanted[i].uniqueID){
                                    // cout << "b is valid" << endl;;
                                    request += b;
                                }
                                else{
                                    // cout << "B is not valid" << endl; 
                                    request += "0";
                                }
                            }
                        }

                        updateFileWanted(buff1, fileWanted, tuniqeID);

                        /// Requesting files
                        if (send(send_fd, request.c_str(), request.size(), 0) == -1){
                            perror("send");
                        }

                        // cout << "[/] Request send " << request << endl;

                        for(int i = 0; i < strlen(buff1); i++){
                            char r = request[i];
                            // cout << "REACHED" << endl;
                            string file = fileWanted[i].fileName ;
                            if(r == '1'){
                                recieveFile(send_fd, directory + "Downloaded/" + file);
                            
                                // cout << "[*] files Recieved " << file << endl;

                                if (send(send_fd, message.c_str(), message.size(), 0) == -1){
                                    perror("send");
                                }    

                                // cout << "[/] useless send " << message << endl;
                            }



                        }


                        /// TO keep order
                        if ((numbytes = recv(send_fd, buff1, MAX_DATA_SIZE - 1, 0)) == -1) {
                            perror("talker: sendto");
                            exit(1);
                        }
                        buff1[numbytes] = '\0';

                        // cout << "[/] useless recieved " << buff1<< endl;



                        
                        if (send(send_fd, message.c_str(), message.size(), 0) == -1){
                            perror("send");
                        }
        
                        // cout << "[*] third sent" << message << endl;
                        

                        strcpy(buff1, "Sanchit");

                        numbytes = recv(send_fd, buff1, MAX_DATA_SIZE - 1, 0);
                        if (numbytes == -1) {
                            close(send_fd);
                            continue;
                        }
                        buff1[numbytes]  = '\0';
                        // cout << "[*] third recived" << buff1 <<  endl;
                        // cout << "files wanted by sender " << buff1 << endl;


                        string reply = "";
                        vector<string> filesWantedBySender = breakDelimmiter(buff1, ',');
                        for(auto file : filesWantedBySender){
                            auto x = find_if(ownedFilesNames.begin(), ownedFilesNames.end(), [file](pair<string, int> const &p){return fileName(p.first) == file;});
                            if(x != ownedFilesNames.end()){
                                reply += to_string((*x).second);
                            }
                            else reply += '0';
                        }

                        
                        // cout << "REPLY SENT " << reply << endl;
                        if (send(send_fd, reply.c_str(), reply.size(), 0) == -1){
                            perror("send");
                        }

                        // cout << "[*] fourth sent" << reply << endl;



                        /// TO keep order 
                        numbytes = recv(send_fd, buff1, MAX_DATA_SIZE - 1, 0);
                        if (numbytes == -1) {
                            close(send_fd);
                            continue;
                        }
                        buff1[numbytes]  =    '\0';



                        // cout << "[/] useless recieve" << buff1 <<  endl;

                        char requestedFiles[MAX_DATA_SIZE];
                        strcpy(requestedFiles, buff1);

                        for(int i = 0; i < strlen(requestedFiles); i++){
                            char r = requestedFiles[i];
                            if(r != '0'){
                                string file = filesWantedBySender[i];
                                // cout << file << endl;
                                auto x = find_if(ownedFilesNames.begin(), ownedFilesNames.end(), [file](pair<string, int> const &p){return fileName(p.first) == file;});
                                // cout << "DO I GET HERE" << endl;
                                file = (*x).first;
                                // cout << "FILE " << directory + file << endl;
                                sendFile(send_fd, directory + file);
                            // cout << "HELLO\n" << endl;

                                // cout << "[*] Send file " << file << endl; 
                                /// TO keep order 
                                numbytes = recv(send_fd, buff1, MAX_DATA_SIZE - 1, 0);
                                if (numbytes == -1) {
                                    close(send_fd);
                                    continue;
                                }
                                buff1[numbytes]  =    '\0'; 

                                // cout << "[/] useless recieve "<<  buff1 << endl;

                            }
                        }
                        
                        // cout << "HELLO THERE "<< endl;
                        // pfds[fd_count] = pfds[fd_count - 1];
                        // fd_count -= 1;
                        // close(send_fd);
                        counter++;
                        alreadyChecked[fd_count - 1] = true;
                    }
                }
            }
        }
    }

    // cout << "REQ" <<endl;
    // int t; cin >> t;

    for(int j = 0; j < sd.size(); j++){

        // cout << "SENDING IN LOOP " << sd[j].first << " " << sd[j].second << endl;  
        int dest_fd = sd[j].second;
        string s = makeInfoMessage(neighbourInfo);
        // cout << "SENDING "<< j << " " << s << endl;
        if(send(dest_fd, s.c_str(),s.length(),0) == -1){
            perror("send");
        }
    }

    for(int j = 0; j < sd.size(); j++){
        int dest_fd = sd[j].second;

        char buff[MAX_DATA_SIZE];
        // cout << "WAITING " << endl;
        int numbytes = recv(dest_fd, buff, MAX_DATA_SIZE - 1, 0);
        if (numbytes == -1) {
            close(dest_fd);
            continue;
        }

        buff[numbytes]  = '\0';
        // cout << "sanchit " << j << " " << numbytes << " "<< buff << endl;
        updateFileWanted2(buff, fileWanted);
        addNeighbourInfo(buff, neighbourInfo);

    }

    int newsd[100];
    vector<vector<string>> filesOnD;
    vector<int> portsRequired;

    for(int i = 0; i < fileWanted.size(); i++){
        if(fileWanted[i].depth == 2){

            int dest = 0;
            for(auto ni : neighbourInfo){
                if(fileWanted[i].uniqueID == ni.uniqueID){
                    dest = ni.port;
                }
            }

            portsRequired.push_back(dest);

        }
    }

    // for(auto i : portsRequired){
    //     cout << "A " << i << endl;
    // }

    auto end = portsRequired.end();
    for(auto it = portsRequired.begin(); it != end; it++){
        end = remove(it+1,end, *it);
    }
    portsRequired.erase(end,portsRequired.end());

    // for(auto i : portsRequired){
    //     cout << "B " << i << endl;
    // }

    usleep(5000000);

    // cout << portsRequired.size() << endl;
    for(int i = 0; i < portsRequired.size(); i++){
        int PORT = portsRequired[i];
        // cout << PORT << endl;
        int sockfd;
        char buff[MAX_DATA_SIZE];
        int rv;
        char s[INET6_ADDRSTRLEN];
        struct addrinfo hints, *servinfo, *p;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET; // set to AF_INET to use IPv4
        hints.ai_socktype = SOCK_STREAM;

        rv = getaddrinfo(local, to_string(PORT).c_str(), &hints, &servinfo);
        if (rv != 0) {
            perror("addr");
            continue;
        }

        for(p = servinfo; p != NULL; p = p->ai_next) {
            sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol);
            if (sockfd == -1) {
                continue;
            }
            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                perror("client: connect");
                continue;
            }
            break;
        }

        if (p == NULL) {
            fprintf(stderr, "talker: failed to create socket\n");
            close(sockfd);
            continue;
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
        // cout << "connecting to " << s << "\n";

        freeaddrinfo(servinfo);

        newsd[i] = sockfd;    

        string mFiles = "";
        int un ;
        for(auto i : neighbourInfo){
            if(PORT == i.port){
                un = i.uniqueID;
                break;
            } 
        }

        vector<string> x;
        for(auto i : fileWanted){
            if(i.uniqueID == un){
                mFiles += i.fileName + ',';
                x.push_back(i.fileName);
            }
        }
        filesOnD.push_back(x);

        if(send(sockfd, mFiles.c_str(),mFiles.length(),0) == -1){
            perror("send");
        }
        // cout << "SENT " << un << " " << mFiles << endl;;
    }
    
    // cout << "HELLO " << endl;


    struct pollfd *pfds1 = (struct pollfd*) malloc(sizeof *pfds * (portsRequired.size()+1));
    pfds1[0].fd =  sockfd1;
    pfds1[0].events = POLLIN;
    int fd_count1 = 1;

    int counter2 = 0;

    while(true){

        // cout << "WAITING HERE" << endl;
        int poll_count = poll( pfds1, fd_count1, 5000);
        if(poll_count == 0){
            // cout << "TIMED OUT\n";
            break;
        }
        if(poll_count == -1){
            perror("poll");
            exit(1);
        }

        for (int i = 0 ; i < fd_count1 ; i++){
            if(pfds1[i].revents & POLLIN){
                if(pfds1[i].fd == sockfd1){
                    char buff[MAX_DATA_SIZE];
                    sin_size = sizeof their_addr;
                    new_fd = accept(sockfd1, (struct sockaddr *) &their_addr, &sin_size);

                    if(new_fd == -1){
                        perror("accept");
                    }
                    else{
                        pfds[fd_count].fd = new_fd;
                        pfds[fd_count].events = POLLIN;
                        fd_count++;

                        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *) &their_addr), s1, sizeof s1);
                    }

                    int numbytes = recv(new_fd, buff, MAX_DATA_SIZE - 1, 0);
                    if (numbytes == -1) {
                        close(new_fd);
                        continue;
                    }

                    buff[numbytes] = '\0';
                    // cout << "-------" << buff << endl; ;
                    
                    vector<string> fi = breakFilesMessage(buff);
                    for(auto i : fi){
                        sendFile2(new_fd, directory + i);
                    }

                    counter2++;
                }
            }
        }
    }

    for(int j= 0; j < portsRequired.size(); j++){
        int dest_fd = newsd[j];

        for(auto i : filesOnD[j]){
            recieveFile2(dest_fd, directory + "Downloaded/" +  i );
        }
    }


    for(int j = 0; j < sd.size(); j++){
        int dest_fd = sd[j].second;
        close(dest_fd);
    }

    printConnections(connections);
    showFileLocations(fileWanted, directory, true);

}