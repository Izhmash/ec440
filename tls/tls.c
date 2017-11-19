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
    void *address;          // page location
    int users;              // number of user threads
};

static struct tls tls_list[MAX_TLS];

// Should semaphores be used?

int is_setup = FALSE;

int get_pages(int size);
void setup();
void page_fault_handler(int sig, siginfo_t *sigi, void *context);

int tls_create(unsigned int size)
{
}

//int tls_write(unsigned int offset, unsigned int length, char *buffer);

//int tls_read(unsigned int offset, unsigned int length, char *buffer);

//int tls_destroy();

//int tls_clone(pthread_t tid);


void page_fault_handler(int sig, siginfo_t *sigi, void *context)
{
}
    

/*
 * Init sig handler, setup tls array
*/
void setup()
{
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = page_fault_handler;

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
