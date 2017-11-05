threads: threads.c
	gcc -Wall -I . -m32 -c -o threads.o threads.c -ggdb -lrt

queue: queue.c
	gcc -Wall -I . -m32 -c -o queue.o queue.c -ggdb -lrt

test-threads: test-threads.c
	gcc -Wall -m32 -o test-threads test-threads.c threads.o queue.o -ggdb -lrt

test-sema: test-sema.c
	gcc -Wall -m32 -o test-sema test-sema.c threads.o queue.o -ggdb -lrt

clean:
	rm threads.o queue.o test-sema test-threads
