#define MAX_THREADS

// Thread control block
static struct tcb {
    int *env;       // Environment pointer
    int (*tcp)();   // Thread function pointer
    int state;      // 0 waiting, 1 running, -1 done
} tcb[MAX_THREADS];

static jmp_buf root_env;
static int thread_num = -1;
