threads: threads.c
	gcc -Wall -I . -m32 -c -o threads.o threads.c -ggdb

test-threads: test-threads.c
	gcc -Wall -m32 -o test-threads test-threads.c threads.o ptr_mangle.c -ggdb -lrt

clean:
	rm threads.o test-threads
