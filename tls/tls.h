#ifndef TLS_H_
#define TLS_H_

int tls_create(unsigned int size);

int tls_write(unsigned int offset, unsigned int length, char *buffer);

int tls_read(unsigned int offset, unsigned int length, char *buffer);

int tls_destroy();

int tls_clone(pthread_t tid);

#endif
