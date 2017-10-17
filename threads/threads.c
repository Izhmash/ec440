#define MAX_THREADS 180
#define STACK_SIZE 32767

// Thread control block
static struct tcb {
    int *env;       // Environment pointer
    int (*tcp)();   // Thread function pointer
    int state;      // 0 waiting, 1 running, -1 done
} tcb[MAX_THREADS];

static jmp_buf root_env;
static int thread_num = -1;

void schedule()
{
    jmp_buf envthread;
    int stack_space[STACK_SIZE];
}
