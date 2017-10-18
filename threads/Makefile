threads: threads.c
	gcc -Wall -I . -m32 -c -o threads.o threads.c

test-threads: test-threads.c
	gcc -Wall -m32 -o test-threads test-threads.c threads.o ptr_mangle.c
