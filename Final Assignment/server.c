#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>

#define MYPORT "2288"   // the port users will be connecting to
#define BACKLOG 10      // how many pending connections queue will hold

int recv_msg(int sockfd, char* msg, size_t buflen);


int main(int argc, char** argv){

    struct addrinfo hints;
    struct addrinfo *res;  // will point to the results   

    // first, load up address structs with getaddrinfo():
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;    // fill in my IP for me
    
    // get ready to connect
    int status = getaddrinfo(NULL, MYPORT, &hints, &res);
    if (status != 0) {
        printf("getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    // make a socket, bind it, and listen on it
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1){
        printf("Error on socket creation\n");
        return 1;
    }

    status = bind(sockfd,res->ai_addr,res->ai_addrlen);
    if (status == -1){
        printf("Error on binding\n");
        return 1;
    }

    status = listen(sockfd,BACKLOG);
    if (status == -1){
        printf("Error on listening\n");
        return 1;
    }

    // now accept an incoming connection
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof their_addr;
    printf("Waiting accept\n");
    int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
    if (new_fd == -1){
        printf("Error on accepting\n");
        return 1;
    }
    printf("Accepted\n");

    // receive messages
    char buf[25];
    int rec_bytes = recv_msg(new_fd,buf,sizeof(buf));
    while (rec_bytes > 0){
        printf("Message: %s\n",buf);
        rec_bytes = recv_msg(new_fd,buf,sizeof(buf));
    }

    close(new_fd);
    close(sockfd);
    return 0;
}

int recv_msg(int sockfd, char* msg, size_t buflen){
    size_t bytes_remain = buflen;
    int recv_bytes = 0;
    int total = 0;
    while (bytes_remain > 0){
        recv_bytes = recv(sockfd,msg+total,bytes_remain,0);
        bytes_remain -= recv_bytes;
        total += recv_bytes;
        if (recv_bytes == 0){
            printf("Connection ended\n");
            break;
        }
        else if (recv_bytes == -1){
            printf("Error on receiving\n");
            break;
        }
    }
    return recv_bytes;
}