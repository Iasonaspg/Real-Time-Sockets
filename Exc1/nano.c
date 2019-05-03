#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

double getSecond();
double getMonotonicSecond();

int main(int argc,char** argv){

    if (argc != 3){
        printf("Invalid number of arguments\n");
        exit(1);
    }
    else{
        int duration = strtof(argv[1],NULL);
        float dt = strtof(argv[2],NULL);
        
        int count = duration/dt;
        double* timestamps = (double*)malloc(count*sizeof(timestamps));

        int dt_sec = (int) dt;
        long dt_nsec = (dt - dt_sec) * 1.e9;
        struct timespec ts;
        ts.tv_sec = dt_sec;
        ts.tv_nsec = dt_nsec;
    
        for (int i=0;i<count;i++){
            nanosleep(&ts,NULL);
            timestamps[i] = getSecond();
        }

        for (int i=1;i<count;i++){
            printf("Difference: %lf\n",timestamps[i]-timestamps[i-1]);
        }
    }
    return 0;
}

double getSecond() {
    struct timeval tp;
    gettimeofday(&tp,NULL);
    return ((double)tp.tv_sec + (double)tp.tv_usec*1.e-6);
}

double getMonotonicSecond(){
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC,&tp);
    return ((double)tp.tv_sec + (double)tp.tv_nsec*1e-9);
}