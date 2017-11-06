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
#define ALRM_TIME   50000
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
    //lock();
    //printf("scheduling!\n");
    sigset_t signal_set;
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGALRM);
    sigprocmask(SIG_BLOCK, &signal_set, NULL);
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
        int last_done = 1;

        // XXX Don't run thread 0 unless it's the last thread
        int i;
        for (i = 1; i < MAX_THREADS; i++) {
            if (thread_map[i] == 1) {
                last_done = 0;
            }
        }

		do {
            cur_thread++;
            if (cur_thread > MAX_THREADS - 1) {
                cur_thread = 0;
                if (!last_done) cur_thread = 1;
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
        //unlock();
		longjmp(pthreads[cur_thread].env, 1);
    } else {
		sigset_t signal_set;
		sigemptyset(&signal_set);
		sigaddset(&signal_set, SIGALRM);
		sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
        //unlock();
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
    pthreads[0].prev_state = EMPTY;
    pthreads[0].exit_code = NULL;
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
	timer.it_interval.tv_usec = ALRM_TIME;
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
	/*sigset_t signal_set;
	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGALRM);
	sigprocmask(SIG_BLOCK, &signal_set, NULL);*/
    lock();

    thread_map[cur_thread] = 0;

    pthreads[cur_thread].exit_code = value_ptr;
    pthreads[cur_thread].state = EXITED;

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
	pthread_t target_state = pthreads[thread].state;
	pthread_t target_merg = pthreads[thread].merger;
	if (target_state != EMPTY && target_state != EXITED) {
        if (target_merg == -1) {
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

struct Semaphore
{
    int val;
    struct Queue *q; 
	int ready;
};

int sem_init(sem_t *sem, int pshared, unsigned value)
{
    struct Semaphore *sema = 
        (struct Semaphore*) malloc(sizeof(struct Semaphore));

    // Save semaphore initial value
    sema->val = value;

    // Init struct
    struct Queue *queue = init_queue(128);
    sema->q = queue;

    sema->ready = 1;

    // Store reference to my struct in sem_t struct
    sem->__align = (long int)sema;  
    return 0;
}

int sem_wait(sem_t *sem)
{
    // Get the real Semaphore struct
    lock();
    struct Semaphore *sema = (struct Semaphore*) sem->__align;

    if (!sema->ready) return -1;

    pthread_t self = pthread_self();

    // if blocked, push onto queue
    if (sema->val == 0) {
        enqueue(sema->q, self);

        // XXX NEXT TWO LINES *MIGHT* BE REALLY BAD
        pthreads[self].state = BLOCKED;
        schedule();
        /*while (sema->val == 0) {
            ;;
        }*/
    } 

    sema->val--;
    unlock();

    return 0;
}

int sem_post(sem_t *sem)
{
    // Get the real Semaphore struct
    struct Semaphore *sema = (struct Semaphore*) sem->__align;
    if (!sema->ready) return -1;

    lock();
    sema->val++;
    if (sema->val > 1) {
        // Return right away
        unlock();
        return 0;
    } else {
        // Increment and wake thread up
        pthread_t thread = dequeue(sema->q);
        if (thread != -1) {
            // If there was a thread waiting...
            pthreads[thread].state = READY;
        }
        unlock();
        schedule();
        // Jump to the sleepy thread that can now wake up
        //longjmp(pthreads[thread].env, 1);
    }
    // Switch context to wake up blocked threads?
    //schedule();

    
    return 0;
}

int sem_destroy(sem_t *sem)
{
    // Get the real Semaphore struct
    struct Semaphore *sema = (struct Semaphore*) sem->__align;
    if (!sema->ready) return -1;

    // Free queue
    free(sema->q);
    // Free semaphore
    free(sema);
    return 0;
}
