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
#include "utils.h"
using namespace std;
namespace fs = std::filesystem;

#define MAX_DATA_SIZE 100
#define BACKLOG 10


int main(int argc, char *argv[])
{
    
    
    if (argc != 3) {
        fprintf(stderr,"Usage: (executable argument1-config-file argument2-directory-path)\n");
        exit(1);
    }

    ///////////////////////////////////////
    //   READING THE DIRECTORY AND PRINTING ALL THE FILE NAMES EXCEPT DOWNLOADED FOLDER 
    ///////////////////////////////////////

    vector<string> ownedFilesNames;

    string directory = argv[2];
    for(const auto file : fs :: directory_iterator(directory)){
        if(!fs::is_directory(file.status())){
            if(getFileName(file.path(),directory) != "Downloaded") {
                cout << getFileName(file.path(), directory) << "\n";
                ownedFilesNames.push_back(getFileName(file.path(), directory));
            }
        }
    }
    
    ///////////////////////////////////////
    // READING THE CONFIG FILE AND STORING ALL THE INFORMATION IN VARIABLES
    ///////////////////////////////////////
    
    int id, port, uniqueID;
    int nNeighbours;
    vector<int> Nid, Nport;
    vector<bool> found;
    int nFileWanted;
    vector<string> fileWanted;
    vector<int> NuniqueID;

    ifstream configFile(argv[1]);
    configFile >> id;
    configFile >> port;
    configFile >> uniqueID;
    configFile >> nNeighbours;

    string message = to_string(id) + "," + to_string(uniqueID) + "," + to_string(port);    

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
        fileWanted.push_back(temp);
    }
    configFile.close();

    vector<pair<string, int>> connections;


    const char* local = "localhost";
    int cnt = 0;

    ///////////////////////////////////////
    // FOR EACH PORT TRYING TO FIND IF IT IS BEING LISTENED ON AND KEEPING TRACK OF WHAT HAVE BEEN SEEN 
    ///////////////////////////////////////
    
    for(auto PORT : Nport){
        int sockfd, numbytes;
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

        freeaddrinfo(servinfo);
    
        

        if ((numbytes = recv(sockfd, buff, MAX_DATA_SIZE - 1, 0)) == -1) {
            // perror("talker: sendto");
            close(sockfd);
            continue;
            exit(1);
        }

    
        // We got a good output remove the 
        buff[numbytes]  = '\0';
        addUniqueId(buff , Nid, NuniqueID, found);
        // cout << showNeighbourUniqeId(buff, PORT) << endl;
        connections.push_back({buff, PORT});

        if (send(sockfd, message.c_str(), message.size(), 0) == -1){
            perror("send");
        }

        cnt++;
        close(sockfd);
    }
    
    if(allNeighboursFound(found)){
        printConnections(connections);
        return 0;
    }


    // None of the ports are already estabilished start listening on the ports
    int sockfd1 , numbytes;
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
        if(counter == fd_size - 1){
            return 0;
        }

        int poll_count = poll( pfds, fd_count, -1);

        if(poll_count == -1){
            perror("poll");
            exit(1);
        }

        for (int i = 0 ; i < fd_count ; i++){

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

                        if ((numbytes = recv(send_fd, buff1, MAX_DATA_SIZE - 1, 0)) == -1) {
                            perror("talker: sendto");
                            exit(1);
                        }
                        
                        buff1[numbytes] = '\0';

                        addUniqueId(buff1, Nid, NuniqueID, found);
                        string b = buff1;
                        int p = stoi(b.substr(b.find_last_of(',') + 1));

                        // cout << showNeighbourUniqeId(buff1, p) << endl;
                        connections.push_back({buff1, p});
                        // pfds[fd_count] = pfds[fd_count - 1];
                        // fd_count -= 1;
                        // close(send_fd);
                        // counter++;
                    }
                }
            }
        }
    }

    printConnections(connections);
}