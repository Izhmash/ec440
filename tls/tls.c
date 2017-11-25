#include <pthread.h>
#include <sys/mman.h>
#include <stdio.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "tls.h"

#define PAGE_SIZE getpagesize()
#define TRUE 1
#define FALSE 0
#define MAX_TLS 128

struct tls {
    pthread_t thread;
    unsigned int size;      // mem size in bytes
    struct page** pages;    // array of page pointers 
};

struct page {
    void *addr;             // page location
    int users;              // number of user threads
};

static struct tls tls_list[MAX_TLS];
int tls_count;

static struct sigaction act;
static int is_setup = FALSE;

int get_pages(int size);
void setup();
void page_fault_handler(int sig, siginfo_t *sigi, void *context);

/*
 * Create thread local storage space
*/
int tls_create(unsigned int size)
{
    if (size < 1) return -1;

    // Check for TLS existence
    pthread_t cur_thread = pthread_self();
    int i;
    for (i = 0; i < MAX_TLS; i++) {
        if (pthread_equal(tls_list[i].thread, cur_thread)) {
            return -1;
        }
    }

    if (!is_setup) {
        setup();
        is_setup = TRUE;
    } 

    if (tls_count == MAX_TLS) return -1;

    struct tls *new_tls = NULL;

    // Add TLS to list
    for (i = 0; i < MAX_TLS; i++) {
        // If TLS space is unused...
        if (tls_list[i].thread == 0) {
            new_tls = &tls_list[i];
            break;
        }
    }

    // Setup TLS
    new_tls->thread = cur_thread;
    new_tls->size = size;
    new_tls->pages = calloc(get_pages(size), sizeof(struct page*));
    
    for (i = 0; i < get_pages(size); i++) {
        void *new_addr = mmap(NULL, PAGE_SIZE, PROT_NONE, MAP_ANONYMOUS
                | MAP_PRIVATE, -1, 0);
        new_tls->pages[i] = calloc(1, sizeof(struct page));
        new_tls->pages[i]->addr = new_addr;
        new_tls->pages[i]->users = 1;
    }

    tls_count++;
    return 0;
}

/*
 * Write to thread local storage
*/
int tls_write(unsigned int offset, unsigned int length, char *buffer)
{
    struct tls *read_tls;

    // Get the current thread's TLS
    int i;
    int cur_thread = pthread_self();
    int has_tls = FALSE;
    for (i = 0; i < MAX_TLS; i++) {
        if (pthread_equal(tls_list[i].thread, cur_thread)) {
            read_tls = &tls_list[i];
            has_tls = TRUE;
            break;
        }
    }

    if (!has_tls) return -1;
    if (read_tls->size < offset + length) return -1;

    // Write the page(s)
    int start_page = offset / PAGE_SIZE;
    int num_pages = (length + (offset % PAGE_SIZE) + PAGE_SIZE - 1) / PAGE_SIZE;
    int write_prog = 0;

    struct page *cur_page;
    for (i = start_page; i < start_page + num_pages; i++) {
        // Unlock page
        mprotect(read_tls->pages[i]->addr, PAGE_SIZE, PROT_WRITE);

        // Copy-on-write for TLS's that share
        if (read_tls->pages[i]->users > 1) {
            // Page is no longer shared
            read_tls->pages[i]->users--;

            // Copy old page to new page
            void *new_addr = NULL;
            new_addr = mmap(NULL, PAGE_SIZE, PROT_WRITE, MAP_ANONYMOUS
                    | MAP_PRIVATE, -1, 0);
            memcpy(new_addr, read_tls->pages[i]->addr, PAGE_SIZE);
            mprotect(read_tls->pages[i]->addr, PAGE_SIZE, PROT_NONE);

            // Overwrite old page reference
            read_tls->pages[i] = calloc(1, sizeof(struct page));
            read_tls->pages[i]->users = 1;
            read_tls->pages[i]->addr = new_addr;
        }

        // Write data
        int bytes = offset % PAGE_SIZE;
        cur_page = read_tls->pages[i];
        if (num_pages == 1) {
            // First check if only 1 page is written
            int write_addr = (int) cur_page->addr + bytes;
            memcpy((void *) write_addr, (void *) buffer, length);
        } else if (i == start_page) {
            // Add offset since 1st page
            int write_addr = (int) cur_page->addr + bytes;
            memcpy((void *) write_addr, (void *) buffer, PAGE_SIZE - bytes);
            write_prog = PAGE_SIZE - bytes;
        } else if (i == start_page + num_pages - 1) {
            // Last page
            int write_addr = (int) cur_page->addr;
            int read_addr = (int) buffer + write_prog;
            memcpy((void *) write_addr, (void *) read_addr, length-write_prog);
        } else {
            // Middle page
            int read_addr = (int) buffer + write_prog;
            memcpy(cur_page->addr, (void *) read_addr, PAGE_SIZE);
            write_prog += PAGE_SIZE;
        }

        // Lock page
        mprotect(cur_page->addr, PAGE_SIZE, PROT_NONE);
    }

    return 0;
}

/*
 * Read from thread local storage
*/
int tls_read(unsigned int offset, unsigned int length, char *buffer)
{
    struct tls *write_tls;

    // Get the current thread's TLS
    int i;
    int cur_thread = pthread_self();
    int has_tls = FALSE;
    for (i = 0; i < MAX_TLS; i++) {
        if (pthread_equal(tls_list[i].thread, cur_thread)) {
            write_tls = &tls_list[i];
            has_tls = TRUE;
            break;
        }
    }

    if (!has_tls) return -1;
    if (write_tls->size < offset + length) return -1;

    // Read the page(s)
    int start_page = offset / PAGE_SIZE;
    int num_pages = (length + (offset % PAGE_SIZE) + PAGE_SIZE - 1) / PAGE_SIZE;
    int read_prog = 0;

    struct page *cur_page;
    for (i = start_page; i < start_page + num_pages; i++) {
        cur_page = write_tls->pages[i];

        // Unlock page
        mprotect(write_tls->pages[i]->addr, PAGE_SIZE, PROT_READ);

        // Read data
        int bytes = offset % PAGE_SIZE;
        if (num_pages == 1) {
            // First check if only 1 page is read
            int read_addr = (int) cur_page->addr + bytes;
            memcpy((void *) buffer, (void *) read_addr, length);
        } else if (i == start_page) {
            // Add offset since 1st page
            int read_addr = (int) cur_page->addr + bytes;
            memcpy((void *) buffer, (void *) read_addr, PAGE_SIZE - bytes);
            read_prog = PAGE_SIZE - bytes;
        } else if (i == start_page + num_pages - 1) {
            // Last page
            int write_addr = (int) buffer + read_prog;
            memcpy((void *) write_addr, cur_page->addr, length - read_prog);
        } else {
            // Middle page
            int write_addr = (int) buffer + read_prog;
            memcpy((void *) write_addr, cur_page->addr, PAGE_SIZE);
            read_prog += PAGE_SIZE;
        }

        // Lock page
        mprotect(write_tls->pages[i]->addr, PAGE_SIZE, PROT_NONE);
    }

    return 0;
}

/*
 * Clone local storage of target pthread
*/
int tls_clone(pthread_t tid)
{
    struct tls *clone_target = NULL;
    struct tls *cur_tls = NULL;

    // Check for TLS existence just as tls_create does
    pthread_t cur_thread = pthread_self();
    int i;
    for (i = 0; i < MAX_TLS; i++) {
        if (pthread_equal(tls_list[i].thread, cur_thread)) {
            return -1;
        }
    }

    // Initialize the current thread's TLS
    int cur_has_tls = FALSE;
    for (i = 0; i < MAX_TLS; i++) {
        if (tls_list[i].thread == 0) {
            cur_tls = &tls_list[i];
            cur_has_tls = TRUE;
            break;
        }
    }
    if (!cur_has_tls) return -1;

    // Save current thread's ID to TLS
    cur_tls->thread = cur_thread;
    
    // Get the clone target thread's TLS
    int target_has_tls = FALSE;
    for (i = 0; i < MAX_TLS; i++) {
        if (pthread_equal(tls_list[i].thread, tid)) {
            clone_target = &tls_list[i];
            target_has_tls = TRUE;
            break;
        }
    }
    if (!target_has_tls) return -1;

    // Prepare new TLS & point to target
    cur_tls->size = clone_target->size;
    int num_pages = get_pages(cur_tls->size);
    cur_tls->pages = calloc(num_pages, sizeof(struct page*));

    for (i = 0; i < num_pages; i++) {
        cur_tls->pages[i] = clone_target->pages[i];
        cur_tls->pages[i]->users++;
    }

    tls_count++;
    return 0;
}

/*
 * Free thread local storage
*/
int tls_destroy()
{
    struct tls *destroy_tls;

    // Get the current thread's TLS
    int i;
    int cur_thread = pthread_self();
    int has_tls = FALSE;
    for (i = 0; i < MAX_TLS; i++) {
        if (pthread_equal(tls_list[i].thread, cur_thread)) {
            destroy_tls = &tls_list[i];
            has_tls = TRUE;
            break;
        }
    }

    if (!has_tls) return -1;

    // Free memory
    for (i = 0; i < get_pages(destroy_tls->size); i++) {
        struct page *cur_page = destroy_tls->pages[i];
        if (cur_page->users > 1) {
            // Remaining users
            cur_page->users--;
        } else {
            // No remaining users
            munmap(cur_page->addr, PAGE_SIZE);
            free(cur_page);
        }
    }
    free(destroy_tls->pages);
    destroy_tls->pages = NULL;

    destroy_tls->thread = -1;
    destroy_tls->size = -1;

    tls_count--;
    return 0;
}

void page_fault_handler(int sig, siginfo_t *sigi, void *context)
{
    // Kill offending thread...
    int tls_related = FALSE;
    int illegal_page = ((unsigned) sigi->si_addr) & ~(PAGE_SIZE - 1);

    int i;
    for (i = 0; i < MAX_TLS; i++) {
        struct tls *cur_tls = &tls_list[i];
        int j;
        for (j = 0; j < get_pages(cur_tls->size); j++) {
            if ((int) cur_tls->pages[j]->addr == illegal_page) {
                // Kill thread if it broke the rules
                tls_destroy();
                pthread_exit(NULL);
            }
        }
    }


    if (!tls_related) { 
        // Raise signal if not related to TLS
        act.sa_sigaction = (void *) SIG_DFL;
        sigaction(SIGSEGV, &act, (void *)page_fault_handler);
        sigaction(SIGBUS, &act, (void *) page_fault_handler);
        raise(sig);
    }
}
    

/*
 * Init sig handler, setup tls array
*/
void setup()
{
    tls_count = 0;

    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = page_fault_handler;

    sigaction(SIGSEGV, &act, NULL);
    sigaction(SIGBUS, &act, NULL);

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
