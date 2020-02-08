#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include "ESPX.h"

#define buff_capacity 2000
#define AEM_count 7
#define BACKLOG 20

static volatile int keepRunning = 1;

typedef struct thread_data{
    struct addrinfo** res;
    char** msg_buf;
    int sock;
    int msg_len;
    short* history;
    int src;
} thr_data;


uint32_t AEM[AEM_count] = {8021,2014,9015,8040,3040,8419,8977};
uint32_t myAEM = 2040;

char IPs[AEM_count][16] = {"10.0.80.21","10.0.20.14","10.0.90.15",
    "10.0.80.40","10.0.30.40","10.0.84.19","10.0.89.77"
};

int sockfd[AEM_count];
int connected[AEM_count];
pthread_t threads[AEM_count],r_threads[AEM_count];
size_t msg_sent[AEM_count];
size_t msg_rcv[AEM_count];
size_t msg_ac_rcv[AEM_count];

size_t buff_size = 0;
int buff_index = -1;

pthread_mutex_t b_size,devs;

int devices = 0;

int main(int argc, char** argv){
    if (argc != 2 ){
        printf("Invalid input number of args\n");
        return 1;
    }
    srand(time(0));
    int msg_len = atoi(argv[1]);
    size_t msg_buf_len = 2*11 + 21 + 3*sizeof("_") + msg_len; // Int32 needs char[11] and Int64 needs char[21] to be stored
    char** buffer = malloc(buff_capacity*sizeof(*buffer));
    for (int i=0;i<buff_capacity;i++){
        buffer[i] = malloc(msg_buf_len*sizeof(*buffer[i]));
    }
    short* history = malloc(buff_capacity*AEM_count*sizeof(*history));
    char temp[msg_buf_len];
    generate_msg(msg_len,temp);
    insert(temp,history,buffer);

    // Prepare addr structs for server side
    struct addrinfo* ser_res;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_STREAM;
    int status = getaddrinfo("10.0.20.40","2288",&hints,&ser_res);
    if (status != 0) {
        printf("getaddrinfo for localhost failed with code: %s\n", gai_strerror(status));
    }
    int ser_sock = socket(ser_res->ai_family,ser_res->ai_socktype,ser_res->ai_protocol);
    if ( ser_sock == -1){
        printf("Error on server socket creation\n");
    }
    status = bind(ser_sock,ser_res->ai_addr,ser_res->ai_addrlen);
    if (status == -1){
        printf("Error on server binding\n");
    }
    status = listen(ser_sock,BACKLOG);
    if (status == -1){
        printf("Error on listening\n");
    }

    // Prepare addr stucts for client side
    struct addrinfo** res = malloc(AEM_count*sizeof(*res));
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_STREAM;
    for (int i=0;i<AEM_count;i++){
        status = getaddrinfo(IPs[i], "2288", &hints, res+i);
        if (status != 0) {
            printf("getaddrinfo for IP: %s failed with code: %s\n", IPs[i], gai_strerror(status));
        }
    }

    // Block sigint on main thread and all of it's children
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    int st = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (st != 0) printf("Error in pthread_sigmask\n");
    thr_data client_data,server_data;
    client_data.res = res;
    client_data.msg_buf = buffer;
    client_data.history = history;
    server_data.msg_buf = buffer;
    server_data.msg_len = strlen(buffer[0])+1;
    server_data.sock = ser_sock;
    server_data.history = history;
    pthread_mutex_init(&b_size,NULL);
    pthread_mutex_init(&devs,NULL);
    pthread_t thread1,thread2;
    pthread_create(&thread1,NULL,client,(void *) &client_data);
    pthread_create(&thread2,NULL,server,(void *) &server_data);

    // Unblock sigint on main thread
    st = pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    if (st != 0) printf("Error in pthread_sigmask\n");
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &sigHandler;
    sigaction(SIGINT, &sa, NULL);
    while (keepRunning){
        generate_msg(msg_len,temp);
        insert(temp,history,buffer);
        int pause = rand() % 5;
        pause++;
        sleep(pause*60);
        int size = buff_size;
        // for (int j=0;j<size;j++){
        //     printf("Buffer: %s\n",buffer[j]);
        // }
        printf("\n");
    }
    pthread_cancel(thread1);
    pthread_cancel(thread2);
    for (int i=0;i<AEM_count;i++){
        printf("Messages sent to %s: %ld\t",IPs[i],msg_sent[i]);
        printf("Messages received from %s: %ld\n",IPs[i],msg_rcv[i]);
    }
    printf("Freeing resources\n");
    for (int i=0;i<AEM_count;i++) close(sockfd[i]);
    close(ser_sock);
    free(res);
    free(buffer);
    pthread_mutex_destroy(&b_size);
    pthread_mutex_destroy(&devs);
    pthread_exit(NULL);
    return 0;
}


void* client(void* dest){
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    thr_data* data = (thr_data*) dest;
    struct addrinfo** res = data->res;
    char** buffer = data->msg_buf;
    thr_data comm_data[AEM_count];
    for (;;){
        for (int i=0;i<AEM_count;i++){
            if (connected[i] != 1){
                sockfd[i] = socket(res[i]->ai_family, res[i]->ai_socktype, res[i]->ai_protocol);
                if (sockfd[i] == -1){
                    printf("Error on socket creation for IP: %s\n",IPs[i]);
                }
                else{
                    int synRetries = 1; // 2 => Timeout ~7s
                    setsockopt(sockfd[i], IPPROTO_TCP, TCP_SYNCNT, &synRetries, sizeof(synRetries));
                    int status = connect(sockfd[i],res[i]->ai_addr,res[i]->ai_addrlen);
                    if (status == -1){
                        // printf("Error on connecting to IP: %s\n",IPs[i]);
                        close(sockfd[i]);
                    }
                    else{
                        connected[i] = 1;
                        comm_data[i].msg_buf = buffer;
                        comm_data[i].sock = i;
                        comm_data[i].history = data->history;
                        pthread_create(&threads[i],NULL,client_handler,(void *)&comm_data[i]);
                        pthread_mutex_lock(&devs);
                        devices++;
                        printf("Devices connected: %d\n",devices);
                        pthread_mutex_unlock(&devs);
                    }
                }
            }
        }
    }
    pthread_exit(NULL);
}

void* server(void* dest){
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    thr_data* data = (thr_data*) dest;
    int ser_sock = data->sock;

    struct sockaddr_storage cli_addr;
    socklen_t addr_size = sizeof(cli_addr);
    char ip[INET_ADDRSTRLEN];
    thr_data comm_data[AEM_count];
    for (;;){
        int new_fd = accept(ser_sock, (struct sockaddr *)&cli_addr, &addr_size);
        if (new_fd == -1){
            printf("Error on accepting\n");
        }
        else{
            // printf("Connection accepted\n");
            struct sockaddr_in* ip_struct = (struct sockaddr_in *) &cli_addr;
            inet_ntop(AF_INET,&(ip_struct->sin_addr),ip,INET_ADDRSTRLEN);
            size_t pos = find_sender(AEM_count,ip);
            if (pos != -1){
                comm_data[pos].sock = new_fd;
                comm_data[pos].src = pos;
                comm_data[pos].msg_buf = data->msg_buf;
                comm_data[pos].msg_len = data->msg_len;
                comm_data[pos].history = data->history;
                pthread_create(&r_threads[pos],NULL,server_handler,(void *)&comm_data[pos]);
            }
        }
    }
    pthread_exit(NULL);
}

void* client_handler(void* data){
    double start = getMonotonicSecond();
    thr_data* inc_data = (thr_data *) data;
    char** buffer = inc_data->msg_buf;
    int sock_pos = inc_data->sock;
    short* history = inc_data->history;
    printf("Connected to %s\n",IPs[sock_pos]);
    int size = buff_size;
    int count = 0;
    for (int i=0;i<size;i++){
        if (history[sock_pos*buff_capacity + i] == 0){
            int s = send_msg(sockfd[sock_pos],buffer[i]);
            // printf("Sending: %s\n",buffer[i]);
            history[sock_pos*buff_capacity + i] = 1;
            count++;
            msg_sent[sock_pos]++;
        }
    }
    printf("Disconnecting from %s after %lf seconds. Messages sent: %d\n",IPs[sock_pos],getMonotonicSecond()-start,count);
    pthread_mutex_lock(&devs);
    devices--;
    printf("Devices connected: %d\n",devices);
    pthread_mutex_unlock(&devs);
    close(sockfd[sock_pos]);
    connected[sock_pos] = 0;
    pthread_exit(NULL);
}

void* server_handler(void* data){
    double start = getMonotonicSecond();
    thr_data* inc_data = (thr_data*) data;
    int sock = inc_data->sock;
    char** buffer = inc_data->msg_buf;
    int msg_len = inc_data->msg_len;
    short* history = inc_data->history;
    int src = inc_data->src;
    char temp[msg_len];
    int recv = 0;
    int count = 0;
    int s = pthread_mutex_lock(&b_size);
    printf("Connected from: %s\n",IPs[src]);
    do{
        recv = recv_msg(sock,temp,msg_len);
        if (recv > 0){
            // printf("Received: %s\n",temp);
            short ac = insert(temp,history,buffer);
            count++;
            msg_rcv[src]++;
            if (ac == 1) msg_ac_rcv[src]++;
        }
    }while (recv > 0);
    printf("Getting disconnected from %s after %lf seconds. Messages received: %d\n",IPs[src],getMonotonicSecond()-start,count);
    pthread_mutex_unlock(&b_size);
    close(sock);
    pthread_exit(NULL);
}

void sigHandler(int dummy){
    keepRunning = 0;
}

size_t find_sender(const size_t length, const char* value){
    for (int i=0;i<length;i++){
        int status = strcmp(value,IPs[i]);
        if (status == 0) return i;
    }
    return -1;
}


/* Communication Functions */
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
    return total;
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


/* Message Functions */
char* random_string(const size_t len){
    char a[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char* str = malloc((len)*sizeof(*str));
    for (size_t i=0;i<len-1;i++){
        str[i] = a[rand() % (sizeof(a)-1)];
    }
    str[len-1] = '\0';
    return str;
}

size_t generate_msg(const size_t message_len, char* buf_msg){
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


/* Buffer Functions */
short insert(char* value, short* history, char** buffer){
    for (int i=0;i<buff_size;i++){
        if (strcmp(buffer[i],value) == 0) return -1;
    }
    if ((buff_size + 1) <= buff_capacity){
        buff_size++;
        buff_index++;
    }
    else{
        buff_index++;
        if (buff_index == buff_capacity) buff_index = 0;
    }
    strcpy(buffer[buff_index],value);
    for (int i=0;i<AEM_count;i++){
        history[i*buff_capacity + buff_index] = 0;
    }
    return 1;
}

/* Timing Functions */
double getMonotonicSecond(){
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC,&tp);
  return ((double)tp.tv_sec + (double)tp.tv_nsec*1e-9);
}