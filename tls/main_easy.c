#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

int tls_create(unsigned int size);
int tls_destroy();
int tls_write(unsigned int offset, unsigned int length, char * buffer);
int tls_read(unsigned int offset, unsigned int length, char * buffer);

int main(int argc, const char* argv[] )
{

	tls_create(4100);
	char buffer[10];
	int i;
	for(i = 0; i < 10; i++) //add to buffer
	{
		buffer[i] = i;
	}
	tls_write(0, 10, buffer);
	
	char buffa3[10];

	tls_read(0, 10, buffa3);
	printf("Managed to read... \n");

	for(i = 0; i < 10; i++)
	{
		printf("buffer3 - should be %d: %d\n", i, buffa3[i]);
	}

	printf("destroy success: %d\n",tls_destroy());
	return 0;
}
