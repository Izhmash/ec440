#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include <semaphore.h>

#define THREAD_CNT 3

int i;
sem_t sema;

void tester()
{
    printf("thread %d says i is %d\n", (int)pthread_self(), i);
}

// waste some more time
void *itsafunc(void *arg)
{
    printf("thread %d says \"Hey it's a function!\"\n", (int)pthread_self());

    sem_wait(&sema);
    i++;
    tester();
    sem_post(&sema);

    return arg;
}

// waste some time 
void *count(void *arg)
{
	unsigned long int c = (unsigned long int)arg;
	int i;
	for (i = 0; i < c; i++) {
		if ((i % 1000) == 0) {
			printf("tid: 0x%x Just counted to %d of %ld\n", \
			(unsigned int)pthread_self(), i, c);
		}
	}
    return arg;
}

int main(int argc, char **argv)
{
	//pthread_t threads[THREAD_CNT];
	//int i;
	//unsigned long int cnt = 10000000;


    //create THREAD_CNT threads
	/*for(i = 0; i<THREAD_CNT; i++) {
		pthread_create(&threads[i], NULL, count, (void *)((i+1)*cnt));
	}

    //join all threads ... not important for proj2
	for(i = 0; i<THREAD_CNT; i++) {
		pthread_join(threads[i], NULL);
	}*/
    //void *val;
    sem_init(&sema, 0, 1);
    pthread_t thread1;
    pthread_t thread2;
    //pthread_t thread3;
    pthread_create(&thread1, NULL, itsafunc, (void *)(10000));
    pthread_create(&thread2, NULL, itsafunc, (void *)(10000));
    //pthread_create(&thread3, NULL, itsafunc, (void *)(10000));

    //pthread_join(thread1, &val);
    //printf("End val = %d\n", (int)val);
    /*int i = 0;
    while(i < 100000) {
        //printf("i: %d\n", i);
        i++;
    }
    printf("i: %d\n", i);*/
    printf("Hey I'm finished!\n");
    sem_destroy(&sema);
    return 0;
}
