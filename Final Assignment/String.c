#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

uint32_t AEM[10] = {8021,8419,7351,4320,2810,4397,1500,8865,9020,7423};
uint32_t myAEM = 9015;

#define buff_size 2000

double cpuSecond(){
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC,&start);
    return ((double)start.tv_sec + (double)(start.tv_nsec*1.e-9));
}

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


int main(int argc, char** argv){
    srand(time(0));
    int msg_len = atoi(argv[1]);
    size_t msg_buf_len = 2*11 + 21 + 3*sizeof("_") + msg_len; // Int32 needs char[11] and Int64 needs char[21] to be stored
    double start = cpuSecond();
    char** buffer = malloc(buff_size*sizeof(*buffer));
    for (int i=0;i<buff_size;i++){
        buffer[i] = malloc(msg_buf_len*sizeof(*buffer[i]));
    }
    printf("Allocation time: %lf\n",cpuSecond()-start);
    for (int i=0;i<buff_size;i++){
        generate_msg(buffer[i],msg_len);
    }
    // printf("Msg: %s\n",buffer[1999]);
    // printf("Msg: %s\n",buffer[1991]);
    // printf("Msg: %s\n",buffer[256]);
    // printf("Msg: %s\n",buffer[19]);
    free(buffer);
    return 0;
}