#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

int main(int argc, char** argv){

    if (argc != 3){
        printf("Invalid number of input arguments\n");
        exit(1);
    }
    else{
        int duration = strtof(argv[1],NULL);
        float dt = strtof(argv[2],NULL);
        // printf("Duration is %d\n",duration);
        // printf("Dt is %f\n",dt);

        int count = duration/dt + 1;
        int dt_sec = (int) dt;
        long dt_nsec = (dt - (int) dt) * 1.e9;
        double* timestamps = (double*)malloc(count*sizeof(timestamps));

        for (int i=0;i<count;i++){
            struct timeval tp;
            gettimeofday(&tp,NULL);
            timestamps[i] = (double)tp.tv_sec + (double)tp.tv_usec*1.e-6;
            struct timespec ts;
            ts.tv_sec = dt_sec;
            ts.tv_nsec = dt_nsec;
            // printf("Time in seconds: %ld\n",ts.tv_nsec);
            nanosleep(&ts,NULL);
        }

        for (int i=1;i<count;i++){
            printf("Difference: %lf\n",timestamps[i]-timestamps[i-1]);
        }

    }

    

    
    return 0;
}