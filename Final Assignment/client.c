#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int send_msg(int sockfd, char* msg);
char* random_string(size_t len);

uint32_t AEM[10] = {8021,8419,7351,4320,2810,4397,1500,8865,9020,7423};
uint32_t myAEM[5] = {9015};
char IPs[10][16] = {8021,8419,7351,4320,2810,4397,1500,8865,9020,7423};

int main(int argc, char** argv){

    char** ring_buffer = (char**)malloc(2000*sizeof(char*));
    for (int i=0;i<2000;i++) ring_buffer[i] = (char*)malloc(278*sizeof(char));

    struct addrinfo hints;
    struct addrinfo *res;  // will point to the results   

    // first, load up address structs with getaddrinfo():
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_STREAM;
        
    // get ready to connect
    int status = getaddrinfo("10.0.90.15", "2288", &hints, &res);
    if (status != 0) {
        printf("getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    // make a socket and connect to it
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1){
        printf("Error on socket creation\n");
        return 1;
    }

    status = connect(sockfd,res->ai_addr,res->ai_addrlen);
    if (status == -1){
        printf("Error on connecting\n");
        return 1;
    }

    // send and receive messages 
    char msg[50];
    int bytes_sent;
    for (int i=0;i<8;i++){
        sprintf(msg,"We have to talk, round %d",i);
        bytes_sent = send_msg(sockfd, msg);
        if (bytes_sent == -1){
            printf("Error on sending\n");
            return 1;
        }
    }

    close(sockfd);
    return 0;
}



// Function that sends an entire message
int send_msg(int sockfd, char* msg){
    int bytes_sent = 0;
    int bytes_remain = strlen(msg)+1; // null terminator
    int total = 0;
    while (bytes_remain > 0){
        bytes_sent = send(sockfd,msg + total,bytes_remain,0);
        if (bytes_sent == -1){
            printf("Error on sending\n");
            break;
        }
        bytes_remain -= bytes_sent;
        total += bytes_sent;
    }
    return bytes_sent;
}

char* random_string(size_t len){
    char a[26] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char* str = malloc((len+1)*sizeof(*str));
    for (size_t i=0;i<len;i++){
        str[i] = a[rand() % (sizeof(a)-1)];
    }
    str[len] = '\0';
    return str;
}

void generate_msg(char* msg){
    uint32_t rand_aem = AEM[rand() % sizeof(AEM)];

}
