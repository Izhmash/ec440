#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ptr_mangle.h>

#define MAX_THREADS 128
#define STACK_SIZE  32767
#define ALRM_TIME   50000
#define SPIDX       4
#define PCIDX       5

// TODO make method for getting new ID

// Thread control block
static struct pthread {
    pthread_t id;
    jmp_buf env;       // Environment pointer
    int (*function)();   // Thread stack pointer
    //uint8_t *stack_addr;
    int state;
#define EMPTY   0
#define READY   1
#define EXITED  2
#define WAITING 3
} pthreads[MAX_THREADS];

jmp_buf root_env;
int num_threads = -1;
pthread_t cur_thread;
int need_sched = 0;
struct sigaction act;


/* Find new runable thread */
void schedule()
{
    need_sched = 0;
    printf("Scheduling!\n");
}

/* handle timer interrupt */
void interrupt(int val)
{
    schedule();
}

void kill_thread(pthread_t pid) {
    // TODO
    num_threads--;
}


void setup_threads()
{
    struct pthread temp;
    int i;
    for (i = 0; i < MAX_THREADS; ++i) {
        pthreads[i] = temp;
    }
    act.sa_handler = interrupt;
    act.sa_flags = SA_NODEFER;
    sigaction (SIGALRM, &act, 0);
    ualarm(ALRM_TIME, 0);
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
    setup_threads(); // XXX
    struct pthread *pt;
    char *stack_space;
    stack_space = malloc(STACK_SIZE);
    if (stack_space == NULL) return -1;
    int tid = num_threads++; // temp
    pt = &pthreads[num_threads];
    pt->id = tid;

    
}

void pthread_exit(void *value_ptr)
{
}

pthread_t pthread_self(void)
{
    return pthreads[cur_thread].id;
}


/*void make_stack(pthread_t tid)
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
}*/
