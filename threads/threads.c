#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_THREADS 128
#define STACK_SIZE 32767
#define SPIDX 4
#define PCIDX 5

// TODO make method for getting new ID

// Thread control block
static struct pthread {
    pthread_t id;
    jmp_buf env;       // Environment pointer
    int (*function)();   // Thread function pointer
    uint8_t *stack_addr;
    int state;      // 0 waiting, 1 running, -1 exit
} pthreads[MAX_THREADS];

static jmp_buf root_env;
static int thread_num = -1;
static volatile pthread_t cur_thread;

void kill_thread(pthread_t pid) {
    // TODO
}

void make_stack(pthread_t tid)
{
    char stack[STACK_SIZE];
    char new_addr;
    if (MAX_THREADS < tid) {
        longjmp(root_env, 1);
    }
    printf ("Thread %d stack base addr: %p\n", tid, &stack);

    if (setjmp(pthreads[tid].env)) {
        printf("Starting thread %d\n", tid);
        pthreads[tid].function();
        printf("Thread %d returned\n", tid);
        kill_thread(tid);
    } else {
        printf("Thread %d created\n", tid);
        pthreads[tid].stack_addr = &new_addr;
        make_stack(tid + 1);
    }
}

void setup_threads()
{
    // if not returning from elsewhere:
    if (!setjmp(root_env)) {
        make_stack(0);
    } 
}

/* create new thread
 * return id
 */
int pthread_create(
	pthread_t *thread, 
    const pthread_attr_t *attr,
	void *(*start_routine) (void *), 
	void *arg) 
{
    //char *stack_space;
    //stack_space = malloc(STACK_SIZE);
}

void pthread_exit(void *value_ptr)
{
}

pthread_t pthread_self(void)
{
}

/* Find new runable thread */
void schedule()
{
    jmp_buf envthread;
}
