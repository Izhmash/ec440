#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ptr_mangle.h>
#include <string.h>

#define MAX_THREADS 128
#define STACK_SIZE  32767
#define ALRM_TIME   50000
#define SPIDX       4
#define PCIDX       5

// TODO make method for getting new ID

// Thread control block
static struct pthread {
    pthread_t id;
    jmp_buf env;        // Environment variables
    int (*function)();  // Thread function pointer
    unsigned long *stack_store;  // 4 bytes?
    int stack_addr;
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
int need_setup = 1;
struct sigaction act;

/* Copy jmp_buf context */
void copy_context(jmp_buf s, jmp_buf d)
{
    int i;
    char *sb, *db;

    sb = (char *) s;
    db = (char *) d;
    for (i = 0; i < sizeof(jmp_buf); i++) {
        *db++ = *sb++;
    }
}


/* Find new runable thread */
void schedule()
{
    need_sched = 0;
    //printf("scheduling!\n");
    //sigaction (SIGALRM, &act, 0);
    //ualarm(ALRM_TIME, 0);
}

/* handle timer interrupt */
void interrupt(int val)
{
    //printf("interrupted\n");
    schedule();
}

void kill_thread(pthread_t pid) {
    // TODO
    num_threads--;
}


void setup_threads()
{
    need_setup--;
    struct pthread temp;
    int i;
    for (i = 0; i < MAX_THREADS; ++i) {
        pthreads[i] = temp;
    }
    act.sa_handler = interrupt;
    act.sa_flags = SA_NODEFER;
    //sigaction (SIGALRM, &act, 0);
    //ualarm(ALRM_TIME, 0);
}

/* create new thread
 * return id
 */
// NOTE: currently starting thread for testing; temp feature
int pthread_create(
	pthread_t *thread, 
    const pthread_attr_t *attr,
	void *(*start_routine) (void *), 
	void *arg) 
{
    if (need_setup) {
        //printf("setting up thread environment\n");
        setup_threads(); // XXX temp?
    }
    //jmp_buf thread_env;

    //struct pthread *pt;
    int temp_thread = 1; // Need to replace with search
    *thread = temp_thread;
    unsigned long *stack_space;
    

    int tid = num_threads++; // XXX temp?
    //pt = &pthreads[num_threads];
    pthreads[temp_thread].id = tid;
    cur_thread =pthreads[temp_thread].id; // XXX temp!!!

    // stack it up
    stack_space = malloc(sizeof(int)*STACK_SIZE/4);
    if (stack_space == NULL) return -1;
    pthreads[temp_thread].stack_store = stack_space;
    pthreads[temp_thread].stack_store[(STACK_SIZE / 4) - 1] = (unsigned long)arg;
    // Need to add exit code?
    
    // reg it up
    //pt->env[0].__jmpbuf[SPIDX] = ptr_mangle((int)&stack_space);
    //pt->env[0].__jmpbuf[PCIDX] = ptr_mangle((int)pt->function);
    pthreads[temp_thread].stack_addr = ptr_mangle(
            (int)pthreads[temp_thread].stack_store + (STACK_SIZE / 4) - 2);
    pthreads[temp_thread].function = ptr_mangle((int)start_routine);
    
    // infinite loop here
    if (setjmp(pthreads[temp_thread].env)) {
        //printf("set jump!\n");
        //(*pt->function)(); // XXX super temporary but it works!!!
        //pt->state = EXITED; // function has returned
    } else {
        // copy thread_env into created thread
        //copy_context(thread_env, pthreads[pt->id].env);
        //memcpy(&pthreads[pt->id].env, &thread_env, sizeof(jmp_buf));
        pthreads[temp_thread].state = READY;
    }
    // FIXME segfault here
    longjmp(pthreads[temp_thread].env, 1); // XXX testing the longjmp
    //printf("%d\n", pt->env[PCIDX]);
    return pthreads[temp_thread].id;
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
