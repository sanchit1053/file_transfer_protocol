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
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm> 
#include <sstream> 
#include "utils.h"
using namespace std;
namespace fs = std::filesystem;

#define MAX_DATA_SIZE 1000
#define BACKLOG 10


int main(int argc, char *argv[]){
    
    
    if (argc != 3) {
        fprintf(stderr,"Usage: (executable argument1-config-file argument2-directory-path)\n");
        exit(1);
    }

    ///////////////////////////////////////
    //   READING THE DIRECTORY AND PRINTING ALL THE FILE NAMES EXCEPT DOWNLOADED FOLDER 
    ///////////////////////////////////////

    vector<pair<string, int>> ownedFilesNames;
    vector<pair<string, int>> connections;

    vector<string> tempFile;
    string directory = argv[2];
    for(const auto file : fs :: directory_iterator(directory)){
        if(!fs::is_directory(file.status())){
            if(getFileName(file.path(), directory) != "Downloaded") {
                // cout << fileName(getFileName(file.path(), directory)) << "\n";
                string FILENAME = getFileName(file.path(), directory);
                tempFile.push_back(fileName(FILENAME));
                ownedFilesNames.push_back({FILENAME, count(FILENAME.begin(), FILENAME.end(), '/') +1});
                // cout <<  FILENAME << count(FILENAME.begin(), FILENAME.end(), '/') +1 << endl;
                // cout << count(FILENAME.begin(), FILENAME.end(), '/') + 1;
            }
        }
    }

    sort(tempFile.begin(), tempFile.end());

    string ownedFiles = "";

    for(auto file : tempFile){
        cout << file << "\n";
        ownedFiles += file + ",";
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

    vector<pair<int,int>> sd(nNeighbours);

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
        fileWanted.push_back({temp, 0, 0});
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
        auto PORT = Nport[i];
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

        if (p == NULL) {
            // fprintf(stderr, "talker: failed to create socket\n");
            close(sockfd);
            continue;
        }



        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
        // cout << "connecting to " << s << "\n";


        sd[i].second = sockfd;

        freeaddrinfo(servinfo);
    
        
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
        // cout << "[+] first recived" << endl;
        connections.push_back({buff, PORT});


        sd[i].first = tuniqeID;

        if (send(sockfd, message.c_str(), message.size(), 0) == -1){
            perror("send");
        }

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



        // cout << "[+] first sent" << endl;

        numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0);
        if (numbytes == -1) {
            close(sockfd);
            continue;
        }
        buff[numbytes]  = '\0';

        // cout << "[+] second recieved" << endl;
        
        string reply = "";
        vector<string> filesWantedByListener = breakDelimmiter(buff, ',');

        for(auto file : filesWantedByListener){
            // cout << file << endl;
            auto x = find_if(ownedFilesNames.begin(), ownedFilesNames.end(), [file](pair<string, int> const &p){return fileName(p.first) == file ;} ) ;
            if( x != ownedFilesNames.end()){
                // cout << (*x).first << " " << (*x).second;
                reply += to_string((*x).second);
            }
            else reply += '0';
        }

        // cout << "reply " << reply;

        if (send(sockfd, reply.c_str(), reply.size(), 0) == -1){
            perror("send");
        }

        // cout << "[+] second sent" << endl;

        numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0);
        if (numbytes == -1) {
            close(sockfd);
            continue;
        }
        buff[numbytes]  = '\0';

        // cout << "[+] third recieved" << endl;

        if (send(sockfd, messagefiles.c_str(), messagefiles.size(), 0) == -1){
            perror("send");
        }

        // cout << "[+] Third sent" << endl;

        if ((numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0)) == -1) {
            perror("talker: sendto");
            exit(1);
        }
        buff[numbytes] = '\0';
        updateFileWanted(buff, fileWanted, tuniqeID);
        
        // cout << "[+] fourth Recieved" << buff << endl;

        // if ((numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0)) == -1) {
        //     perror("talker: sendto");
        //     exit(1);
        // }
        // buff[numbytes] = '\0';
        // cout << "RE " << buff << endl;

        // updateFileWanted2(buff, fileWanted);

        // for(auto file : fileWanted){
        //     cout << file.fileName << file.uniqueID <<" " << file.depth << endl;
        // }

        cnt++;
        // close(sockfd);
    }
    
    // if(allNeighboursFound(found)){
    //     showFileLocations(fileWanted, directory, false);
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
    // int fd_size = nNeighbours ;
    struct pollfd *pfds = (struct pollfd*) malloc(sizeof *pfds * fd_size);

    memset(&hints1, 0, sizeof hints1);
    hints1.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints1.ai_socktype = SOCK_STREAM;
    hints1.ai_flags = AI_PASSIVE;

    int rv1 = getaddrinfo(NULL, to_string(port).c_str(), &hints1, &servinfo1);
    if (rv1 != 0) {
        // fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        cout << "-> getaddrinfo " << gai_strerror(rv1) <<endl;
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

    while(true){        
        // cout << fd_size << endl;
        if(counter == fd_size - 1){
            // cout << "HEHEHEH " << endl;
            break;
        }

        int poll_count = poll( pfds, fd_count, -1);

        if(poll_count == -1){
            perror("poll");
            exit(1);
        }

        for (int i = 0 ; i < fd_count ; i++){

            if(pfds[i].revents & POLLIN){

                if(pfds[i].fd == sockfd1){
                    // cout << "HHDDDD" << endl;
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

                        // cout << "[*] first sent" << endl;

                        int numbytes;
                        if ((numbytes = recv(send_fd, buff1, MAX_DATA_SIZE - 1, 0)) == -1) {
                            perror("talker: sendto");
                            exit(1);
                        }
                        buff1[numbytes] = '\0';
                        
                        addUniqueId(buff1, Nid, NuniqueID, found);
                        int tid, tuniqeID;
                        breakMessage(buff1, &tid, &tuniqeID);

                        for(int i = 0; i < sd.size(); i++){
                            if(Nid[i] == tid){
                                sd[i].first = NuniqueID[i];
                                sd[i].second = send_fd;
                            }
                        }

                        string b = buff1;
                        int p = stoi(b.substr(b.find_last_of(',') + 1));
                        // cout << showNeighbourUniqeId(buff1, p) << endl;
                        connections.push_back({buff1, p});

                        // cout << "[*] first recieved" << endl;

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

                        // cout << "[*] second sent" << endl;

                        if ((numbytes = recv(send_fd, buff1, MAX_DATA_SIZE - 1, 0)) == -1) {
                            perror("talker: sendto");
                            exit(1);
                        }
                        buff1[numbytes] = '\0';
                        updateFileWanted(buff1, fileWanted, tuniqeID );
                        
                        // cout << "[*] second recived" << endl;
                        
                        if (send(send_fd, message.c_str(), message.size(), 0) == -1){
                            perror("send");
                        }
        
                        // cout << "[*] third sent" << endl;
                        
                        numbytes = recv(send_fd, buff1, MAX_DATA_SIZE - 1, 0);
                        if (numbytes == -1) {
                            close(send_fd);
                            continue;
                        }
                        buff1[numbytes]  =    '\0';
                        // cout << "[*] third recived" << endl;


                        string reply = "";
                        vector<string> filesWantedBySender = breakDelimmiter(buff1, ',');
                        for(auto file : filesWantedBySender){
                            // cout << file << endl;
                            auto x = find_if(ownedFilesNames.begin(), ownedFilesNames.end(), [file](pair<string, int> const &p){return fileName(p.first) == file ;} );
                            if(x != ownedFilesNames.end()){
                                // cout << (*x).first << " " << (*x).second << to_string((*x).second);
                                reply += to_string((*x).second);
                            }
                            else reply += '0';
                        }
                        
                        // cout << "reply" << reply << endl;

                        if (send(send_fd, reply.c_str(), reply.size(), 0) == -1){
                            perror("send");
                        }

                        // cout << "[*] third sent" << endl;
                        
                        // pfds[fd_count] = pfds[fd_count - 1];
                        // fd_count -= 1;
                        // close(send_fd);
                        counter++;
                    }
                }
            }
        }
    }


    // cout << fd_count << endl;
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


    }

    for(int j = 0; j < sd.size(); j++){
        int dest_fd = sd[j].second;
        close(dest_fd);
    }






    printConnections(connections);
    showFileLocations(fileWanted, directory, false);

}