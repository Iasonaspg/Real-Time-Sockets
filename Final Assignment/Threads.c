#include <stdio.h>
#include <pthread.h>

long counter = 0;


void *add_one(){
    counter++;
    long tid = pthread_self();
    printf("Counter from %ld: %ld\n",tid,counter);
}


int main(int argc, char** argv){
    pthread_t thread1, thread2;
    counter++;
    pthread_create(&thread1,NULL,add_one,NULL);
    pthread_join(thread1,NULL);
    printf("Counter from main: %ld\n",counter);
    
    // pthread_t thread1, thread2;
    // pthread_create(&thread1,NULL,(void *) do_one_thing,(void *) &r1);
    // pthread_create(&thread2,NULL,(void *) do_another_thing,(void *) &r2);
    // pthread_join(thread1,NULL);
    // pthread_join(thread2,NULL);

    return 0;
}