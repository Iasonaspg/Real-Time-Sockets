#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "ESPX.h"

#define buff_size 2000
#define AEM_count 11


uint32_t AEM[AEM_count] = {8021,8419,7351,4320,2810,4397,1500,8865,9020,7423,9015};
uint32_t myAEM = 9015;

char IPs[AEM_count][16] = {"10.0.80.21","10.0.84.19","10.0.73.51","10.0.43.20",
    "10.0.28.10","10.0.43.97","10.0.15.00","10.0.88.65","10.0.90.20","10.0.74.23","10.0.90.15"
};

int sockfd[AEM_count];
int connected[AEM_count];

typedef struct thread_data{
    struct addrinfo** res;
} thr_data;


int main(int argc, char** argv){
    if (argc !=2 ){
        printf("Invalid input number of args\n");
        return 1;
    }

    srand(time(0));
    int msg_len = atoi(argv[1]);
    size_t msg_buf_len = 2*11 + 21 + 3*sizeof("_") + msg_len; // Int32 needs char[11] and Int64 needs char[21] to be stored
    char** buffer = malloc(buff_size*sizeof(*buffer));
    for (int i=0;i<buff_size;i++){
        buffer[i] = malloc(msg_buf_len*sizeof(*buffer[i]));
    }
    generate_msg(buffer[4],msg_len);


    // Prepare addr structs
    struct addrinfo hints;
    struct addrinfo** res = malloc(AEM_count*sizeof(*res));
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_STREAM;
    for (int i=0;i<AEM_count;i++){
        int status = getaddrinfo(IPs[i], "2288", &hints, res+i);
        if (status != 0) {
            printf("getaddrinfo for IP: %s failed with code: %s\n", IPs[i], gai_strerror(status));
        }
        else{
            sockfd[i] = socket(res[i]->ai_family, res[i]->ai_socktype, res[i]->ai_protocol);
            if (sockfd[i] == -1){
                printf("Error on socket creation for IP: %s\n",IPs[i]);
            }
            else{
                int synRetries = 1; // 2 => Timeout ~7s
                setsockopt(sockfd[i], IPPROTO_TCP, TCP_SYNCNT, &synRetries, sizeof(synRetries));
            }
        }
    }
    thr_data client_data;
    client_data.res = res;
    pthread_t thread1;
    pthread_create(&thread1,NULL,client,(void *) &client_data);


    pthread_join(thread1,NULL); // Before free!!
    free(res);
    free(buffer);
    pthread_exit(NULL);
    return 0;
}


void* client(void* dest){
    thr_data* data = (thr_data*) dest;
    struct addrinfo** res = data->res;
    for (;;){
        for (int i=0;i<AEM_count;i++){
            if ((sockfd[i] != -1) && (connected[i] != 1)){
                int status = connect(sockfd[i],res[i]->ai_addr,res[i]->ai_addrlen);
                if (status == -1){
                    printf("Error on connecting to IP: %s\n",IPs[i]);
                }
                else{
                    printf("Connection succeeded\n");
                    connected[i] = 1;
                }
            }
        }
    }
    pthread_exit(NULL);
}







/* Message Functions */
char* random_string(size_t len){
    char a[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char* str = malloc((len)*sizeof(*str));
    for (size_t i=0;i<len-1;i++){
        str[i] = a[rand() % (sizeof(a)-1)];
    }
    str[len-1] = '\0';
    return str;
}

size_t generate_msg(char* buf_msg, size_t message_len){
    // Choose random AEM and convert to string
    int rand_pos = rand() % (sizeof(AEM)/sizeof(*AEM));
    uint32_t r_aem = AEM[rand_pos];
    char send_aem[11], recv_aem[11], timestamp[21];
    int s_bytes = sprintf(send_aem,"%u",myAEM);
    int r_bytes = sprintf(recv_aem,"%u",r_aem);

    // Get timestamp as string
    struct timespec tm;
    clock_gettime(CLOCK_REALTIME ,&tm);
    int time_bytes = sprintf(timestamp,"%lu",tm.tv_sec);

    int length = s_bytes + r_bytes + time_bytes + 3*sizeof("_") + message_len;
    char* message = malloc(length*sizeof(*message));
    strcpy(message,send_aem);
    strcat(message,"_");
    strcat(message,recv_aem);
    strcat(message,"_");
    strcat(message,timestamp);
    strcat(message,"_");
    char* random_str = random_string(message_len);
    strcat(message,random_str);
    strcpy(buf_msg,message);
    free(random_str);
    free(message);
    return strlen(buf_msg);
}