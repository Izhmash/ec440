#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


int main() {
    int i;
    char bytes[64];
    char num[32];
    int fd = open("/dev/adder_driver", O_RDWR);
    for (i = 0; i < 10; i++) {
	sprintf(num, "%d", i);
        write(fd, num, strlen(num));
    }
    ssize_t size = read(fd, &bytes, 64);
    printf("Read string: ");
    for (i = 0; i < size; i++) {
        printf("%c", bytes[i]);
    }
    printf("\n");
    return 0;
}
