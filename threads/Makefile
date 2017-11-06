threads: threads.c
	gcc -Wall -I . -m32 -c -o threads.o threads.c -ggdb -lrt

test-threads: test-threads.c
	gcc -Wall -m32 -o test-threads test-threads.c threads.o -ggdb -lrt

test-sema: test-sema.c
	gcc -Wall -m32 -o test-sema test-sema.c threads.o -ggdb -lrt

clean:
	rm threads.o test-sema test-threads
