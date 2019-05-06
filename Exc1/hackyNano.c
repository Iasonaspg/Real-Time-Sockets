#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>


double getSecond();
double getMonotonicSecond();
void writeToFile(char* filename, double* times, int len);

int main(int argc, char** argv){

  if (argc != 3){
    printf("Invalid number of input parameters\n");
    exit(1);
  }
  else{
    int duration = strtof(argv[1],NULL);
    double dt = strtof(argv[2],NULL);

    int count = duration/dt;
    double* times = (double*)malloc(count*sizeof(*times));

    int dt_sec = (int) dt;
    long dt_nsec = (dt - dt_sec) * 1.e9;

    double t1 = getSecond();
    for (int i=0;i<count;i++){
      double t2 = getSecond();
      double remain = (t1 + dt - t2)/1.001;
      int remain_sec = (int) remain;
      long remain_nsec = (remain - remain_sec) * 1.e9;
      struct timespec ts;
      ts.tv_sec = remain_sec;
      ts.tv_nsec = remain_nsec;
      nanosleep(&ts,NULL);
      for (;;){
        double t3 = getSecond();
        if (t3-t1>=dt){
          t1 = t3;
          break;
        } 
      }
      times[i] = t1;
    }

    // double* difs = (double*)malloc((count-1)*sizeof(*difs));
    // for (int i=1;i<count;i++){
    //   difs[i-1] = times[i]-times[i-1];
    //   printf("Difference: %lf\n",times[i]-times[i-1]);
    // }

    writeToFile("./hacky.txt",times,count);
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
