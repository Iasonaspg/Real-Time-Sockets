#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

double getMonotonicSecond();
double getSecond();
void writeToFile(char* filename, double* times, int len);

void timer_handler(int signum)
{
    static int count = 0;
    ++count;
}

int main(int argc, char** argv){

    if (argc != 3){
        printf("Invalid number of input arguments\n");
        exit(1);
    }
    else{
        int duration = strtof(argv[1],NULL);
        float dt = strtof(argv[2],NULL);
        
        int count = duration/dt;
        double* timestamps = (double*)malloc(count*sizeof(*timestamps));

        int dt_sec = (int) dt;
        long dt_usec = (dt - dt_sec) * 1.e6;
        
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = &timer_handler;
        sigaction(SIGALRM, &sa, NULL);
        
        struct itimerval timer;
        timer.it_value.tv_sec = dt_sec;
        timer.it_value.tv_usec = dt_usec;
        timer.it_interval.tv_sec = dt_sec;
        timer.it_interval.tv_usec = dt_usec;
        setitimer(ITIMER_REAL, &timer, NULL);

        for (int i=0;i<count;i++){
            sleep(dt_sec+1);
            timestamps[i] = getSecond();
        }

        // double* difs = (double*)malloc((count-1)*sizeof(*difs));
        // for (int i=1;i<count;i++){
        //   difs[i-1] = timestamps[i]-timestamps[i-1];
        //   printf("Difference: %lf\n",timestamps[i]-timestamps[i-1]);
        // }
    
        writeToFile("./sleepTimer.txt",timestamps,count);
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

void writeToFile(char* filename, double* times, int len){
  FILE* fp1;
  fp1 = fopen(filename,"w");

  for (int i=0;i<len;i++){
    fprintf(fp1,"%lf\n",times[i]);
  }
}
