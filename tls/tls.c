#include <pthread.h>
#include <sys/mman.h>
#include <stdio.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include "tls.h"

#define PAGE_SIZE getpagesize()
#define TRUE 1
#define FALSE 0
#define MAX_TLS 256

struct tls {
    pthread_t thread;
    unsigned int size;      // mem size in bytes
    struct page* pages[];   // array of page pointers 
};

struct page {
    void *addr;             // page location
    int users;              // number of user threads
};

static struct tls tls_list[MAX_TLS];

// Should semaphores be used?

static struct sigaction act;
static int is_setup = FALSE;

int get_pages(int size);
void setup();
void page_fault_handler(int sig, siginfo_t *sigi, void *context);

int tls_create(unsigned int size)
{
    if (size < 1) return -1;

    // Check for TLS existence
    int i;
    for (i = 0; i < MAX_TLS; i++) {
        if (tls_list[i].thread == pthread_self()) {
            return -1;
        }
    }

    if (!is_setup) {
        setup();
        is_setup = TRUE;
    } 

    return 0;
}

int tls_write(unsigned int offset, unsigned int length, char *buffer)
{
    return 0;
}

int tls_read(unsigned int offset, unsigned int length, char *buffer)
{
    return 0;
}

int tls_destroy()
{
    return 0;
}

int tls_clone(pthread_t tid)
{
    return 0;
}


void page_fault_handler(int sig, siginfo_t *sigi, void *context)
{
    // Kill offending thread...
    int tls_related = FALSE;
    int illegal_page = ((unsigned int) sigi->si_addr) & ~(PAGE_SIZE - 1);

    int i;
    for (i = 0; i < MAX_TLS; i++) {
        struct tls *cur_tls = &tls_list[i];
        int j;
        for (j = 0; j < get_pages(cur_tls->size); j++) {
            if (cur_tls->pages[j]->addr == illegal_page) {
                // Kill thread if it broke the rules
                tls_destroy();
                pthread_exit(NULL);
            }
        }
    }


    if (!tls_related) { 
        // Raise signal if not related to TLS
        act.sa_sigaction = SIG_DFL;
        sigaction(SIGSEGV, &act, (void *)page_fault_handler);
        //sigaction(SIGBUS, &act, (void *) page_fault_handler);
        raise(sig);
    }
}
    

/*
 * Init sig handler, setup tls array
*/
void setup()
{
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = page_fault_handler;

    sigaction(SIGSEGV, &act, NULL);
    //sigaction(SIGBUS, &act, NULL);

    int i;
    for(i = 0; i < MAX_TLS; i++) {
        tls_list[i].thread = 0;
    }
}

/*
 * Return byte size in pages
*/
int get_pages(int size)
{
    return (size + PAGE_SIZE - 1) / PAGE_SIZE;
}
