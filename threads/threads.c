#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAX_THREADS 128
#define STACK_SIZE  32767
#define ALRM_TIME   50000
#define SPIDX       4
#define PCIDX       5

// TODO make method for getting new ID

// Thread control block
struct pthread {
    pthread_t id;
    jmp_buf env;        // Environment variables
    //int (*function)();  // Thread function pointer
    int function;  // Thread function pointer
    unsigned long *stack_store;  // 4 bytes?
    int stack_addr;
    int state;
    int needs_setup;
#define EMPTY   0
#define READY   1
#define EXITED  2
#define WAITING 3
} pthreads[MAX_THREADS];

jmp_buf root_env;
int num_threads = -1;
pthread_t cur_thread = 0;
pthread_t last_thread = 0;
static int need_sched = 0;
static int need_setup = 1;
struct sigaction act;
static int thread_map[MAX_THREADS] = {0};

int ptr_mangle(int p);

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
    if (!setjmp(pthreads[cur_thread].env)) {
        // TODO: clear stack space
        
        while(pthreads[cur_thread].state != READY) {
            if (cur_thread > MAX_THREADS + 1) {
                cur_thread = 0;
            }
            cur_thread++;
        }
        if (pthreads[cur_thread].needs_setup == 1) {
			pthreads[cur_thread].env[0].__jmpbuf[PCIDX] =
				pthreads[cur_thread].function;
        	pthreads[cur_thread].env[0].__jmpbuf[SPIDX] =
				pthreads[cur_thread].stack_addr;
			pthreads[cur_thread].state = READY; 
			pthreads[cur_thread].needs_setup = 0;
        }    
    	sigaction (SIGALRM, &act, 0);
    	ualarm(ALRM_TIME, 0);
		longjmp(pthreads[cur_thread].env, 1);
    }
    sigaction (SIGALRM, &act, 0);
    ualarm(ALRM_TIME, 0);
}

/* handle timer interrupt */
void interrupt(int val)
{
    //printf("interrupted\n");
    schedule();
}

void setup_threads()
{
    need_setup--;
    num_threads = 1;
    /*for (i = 0; i < MAX_THREADS; ++i) {
        pthreads[i] = temp;
    }*/
    act.sa_handler = interrupt;
    act.sa_flags = SA_NODEFER;

    // pthreads[0] is main thread
    pthreads[0].id = 0;
    pthreads[0].stack_store = NULL;
    pthreads[0].state = READY;
    pthreads[0].needs_setup = 1;

    setjmp(pthreads[0].env); // ready root thread
    sigaction (SIGALRM, &act, 0);
    ualarm(ALRM_TIME, 0);
	schedule();
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
    int temp_thread;
    // get new id
    int i = last_thread;
    while (1) {
       if (i >= MAX_THREADS) {
           i = 0;
       } 

       if (thread_map[i] == 0) {
           temp_thread = i;
           break;
       }
    }
    int tid = temp_thread;
    last_thread = temp_thread;
    *thread = temp_thread;
    //unsigned long *stack_space;

    num_threads++; // XXX need to replace
    pthreads[temp_thread].id = tid;
    pthreads[temp_thread].state = EMPTY;

    // stack it up
    //if (stack_space == NULL) return -1;
    pthreads[temp_thread].stack_store = malloc(sizeof(int)*STACK_SIZE/4);
    pthreads[temp_thread].stack_store[STACK_SIZE / 4 - 2] 
        = (unsigned int)pthread_exit;
    pthreads[temp_thread].stack_store[STACK_SIZE / 4 - 1] 
        = (unsigned long)arg;
    
    // reg it up
    pthreads[temp_thread].stack_addr = ptr_mangle(
            (int)(pthreads[temp_thread].stack_store + (STACK_SIZE / 4) - 2));
    pthreads[temp_thread].function = ptr_mangle((int)start_routine);
    
    if (setjmp(pthreads[temp_thread].env)) {
        //printf("set jump!\n");
    } else {
    }
    /*// XXX These two will go in the scheduler
    pthreads[temp_thread].env[0].__jmpbuf[SPIDX] = pthreads[temp_thread].stack_addr;
    pthreads[temp_thread].env[0].__jmpbuf[PCIDX] = pthreads[temp_thread].function;
    longjmp(pthreads[temp_thread].env, 1); // XXX testing the longjmp*/
    pthreads[temp_thread].state = READY;
    pthreads[temp_thread].needs_setup = 1;
    if (need_setup) {
        //printf("setting up thread environment\n");
        setup_threads();
    }
    return 0;
}

void pthread_exit(void *value_ptr)
{
    num_threads--;
    thread_map[cur_thread] = 0;
    sigaction (SIGALRM, &act, 0);
    ualarm(0, 0); // turn off alarm temporarily

    pthreads[cur_thread].state = EXITED;
    schedule();
    __builtin_unreachable();
}

pthread_t pthread_self(void)
{
    return pthreads[cur_thread].id;
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
