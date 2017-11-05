#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "queue.h"

#define MAX_THREADS 128
#define STACK_SIZE  32767
#define ALRM_TIME   50
#define SPIDX       4
#define PCIDX       5

// Thread control block
struct pthread {
    pthread_t id;
    jmp_buf env;        // Environment variables
    //int (*function)();  // Thread function pointer
    int function;  // Thread function pointer
    unsigned long *stack_store;  // 4 bytes?
    int stack_addr;
    int state;
    int prev_state;
	void *exit_code;
    pthread_t merger;
    //int needs_setup;
#define EMPTY   0
#define READY   1
#define EXITED  2
#define WAITING 3
#define SETUP	4
#define BLOCKED	5
}; 

static struct pthread pthreads[MAX_THREADS];
void schedule();
void setup_threads();

//jmp_buf root_env;
static int num_threads = 0;
static pthread_t cur_thread = 0;
static pthread_t last_thread = 0;
static int need_setup = 1;
static int thread_map[MAX_THREADS] = {0};

static int ptr_mangle(int p);
void interrupt(int val);
void pthread_exit_wrapper();
int pthread_join(pthread_t thread, void **value_ptr);
void lock();
void unlock();

/* Copy jmp_buf context */
/*void copy_context(jmp_buf s, jmp_buf d)
{
    int i;
    char *sb, *db;

    sb = (char *) s;
    db = (char *) d;
    for (i = 0; i < sizeof(jmp_buf); i++) {
        *db++ = *sb++;
    }
}*/


/* Find new runable thread */
void schedule()
{
    //printf("scheduling!\n");
    if (!setjmp(pthreads[cur_thread].env)) {
       
        // Free stacks 
        if (!cur_thread) {
            int i;
            for (i = 0; i < MAX_THREADS; i++) {

                if (pthreads[i].state == EXITED) {
                    if (pthreads[i].stack_store != NULL) {
                        free(pthreads[i].stack_store);
                    }
                    pthreads[i].state = EMPTY;
                }
            }
        }
        // Find the next thread    
		do {
            cur_thread++;
            if (cur_thread > MAX_THREADS - 1) {
                cur_thread = 0;
            }
        } while(pthreads[cur_thread].state != READY
				&& pthreads[cur_thread].state != SETUP);
        // Thread 0 shouldn't pass this
        if (pthreads[cur_thread].state == SETUP) {
			pthreads[cur_thread].env[0].__jmpbuf[PCIDX] =
				pthreads[cur_thread].function;
        	pthreads[cur_thread].env[0].__jmpbuf[SPIDX] =
				pthreads[cur_thread].stack_addr;
			pthreads[cur_thread].state = READY; 
			//pthreads[cur_thread].needs_setup = 0;
        }    
		sigset_t signal_set;
		sigemptyset(&signal_set);
		sigaddset(&signal_set, SIGALRM);
		sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
		longjmp(pthreads[cur_thread].env, 1);
    } else {
		sigset_t signal_set;
		sigemptyset(&signal_set);
		sigaddset(&signal_set, SIGALRM);
		sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
    }
}

/* handle timer interrupt */
void interrupt(int val)
{
    //printf("interrupted\n");
    schedule();
}

void setup_threads()
{
    need_setup = 0;


    // pthreads[0] is main thread
    pthreads[0].id = 0;
    pthreads[0].stack_store = NULL;
    pthreads[0].state = READY;
    pthreads[0].merger = -1;

    setjmp(pthreads[0].env); // ready root thread
    num_threads++;
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_sigaction = schedule;
	sigaction(SIGALRM, &act, NULL);

    struct itimerval timer;
	timer.it_value.tv_usec = 10;
	timer.it_value.tv_sec = 0;
	timer.it_interval.tv_usec = ALRM_TIME * 1000;
	timer.it_interval.tv_sec = 0;	
	setitimer(ITIMER_REAL, &timer, NULL);
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
    int temp_thread;
    // get new id
    thread_map[0] = 1;
    int i = last_thread;
    while (i <= MAX_THREADS) {
       if (i >= MAX_THREADS) {
           i = 0;
       } 

       if (thread_map[i] == 0) {
           temp_thread = i;
           break;
       }
       i++;
    }
	
	if (i == MAX_THREADS) {
		perror("Thread limit reached.\n");
		exit(1);
	}
    int tid = temp_thread;
    last_thread = temp_thread;
    *thread = temp_thread;

    pthreads[temp_thread].id = tid;
    pthreads[temp_thread].state = EMPTY;
    pthreads[temp_thread].merger = -1;
    pthreads[temp_thread].exit_code = NULL;

    // stack it up
    pthreads[temp_thread].stack_store = malloc(sizeof(int)*STACK_SIZE/4);
    pthreads[temp_thread].stack_store[STACK_SIZE / 4 - 1] 
        = (unsigned long)arg;
    pthreads[temp_thread].stack_store[STACK_SIZE / 4 - 2] 
        = (unsigned int)pthread_exit_wrapper;
    
    // reg it up
    pthreads[temp_thread].stack_addr = ptr_mangle(
            (int)(pthreads[temp_thread].stack_store + (STACK_SIZE / 4) - 2));
    pthreads[temp_thread].function = ptr_mangle((int)start_routine);
    
    setjmp(pthreads[temp_thread].env);
    num_threads++;
    thread_map[temp_thread] = 1;
    pthreads[temp_thread].state = SETUP;

    if (need_setup) {
        //printf("setting up thread environment\n");
        setup_threads();
    }
    return 0;
}

void pthread_exit(void *value_ptr)
{
	sigset_t signal_set;
	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGALRM);
	sigprocmask(SIG_BLOCK, &signal_set, NULL);

    thread_map[cur_thread] = 0;

    pthreads[cur_thread].state = EXITED;
    pthreads[cur_thread].exit_code = value_ptr;

    if (pthreads[pthreads[cur_thread].merger].state == BLOCKED) {
        if (pthreads[cur_thread].merger >= 0) {
            pthreads[pthreads[cur_thread].merger].state
                = pthreads[pthreads[cur_thread].merger].prev_state;
        }
    }
    num_threads--;
    schedule();
    __builtin_unreachable();
}

pthread_t pthread_self(void)
{
    if (cur_thread) {
        return pthreads[cur_thread].id;
    }
    return -1;
}

int pthread_join(pthread_t thread, void **value_ptr)
{
	value_ptr = &pthreads[thread].exit_code;	
	pthread_t cur_state = pthreads[thread].state;
	pthread_t cur_merg = pthreads[thread].merger;
	if (cur_state != EMPTY && cur_state != EXITED) {
        if (cur_merg == -1) {
            pthreads[cur_thread].prev_state = pthreads[cur_thread].state;
            pthreads[cur_thread].state = BLOCKED;
            pthreads[thread].merger = pthreads[cur_thread].id;
            schedule();
        }
	}
    return 0;
}

// Pause scheduling alarm
void lock()
{
    sigset_t signal_set;
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGALRM);
    sigprocmask(SIG_BLOCK, &signal_set, NULL);
}

// Start scheduling alarm
void unlock()
{
    sigset_t signal_set;
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
}

void pthread_exit_wrapper()
{
	unsigned int res;
	asm("movl %%eax, %0\n":"=r"(res));
	pthread_exit((void *) res);
}

int ptr_mangle(int p)
{
	unsigned int ret;
	asm(" movl %1, %%eax;\n"
	" xorl %%gs:0x18, %%eax;\n"
	" roll $0x9, %%eax;\n"
	" movl %%eax, %0;\n"
	: "=r"(ret)
	: "r"(p)
	: "%eax"
	);
	return ret;
}

typedef struct {
    int value;
    struct Queue *q; 
	int ready;
} Semaphore;

int sem_init(sem_t *sem, int pshared, unsigned value)
{
    return 0;
}

int sem_wait(sem_t *sem)
{
    return 0;
}

int sem_post(sem_t *sem)
{
    return 0;
}

int sem_destroy(sem_t *sem)
{
    return 0;
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
