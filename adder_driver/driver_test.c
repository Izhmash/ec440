#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


int main() {
    int i;
    char bytes[64];
    char num[32];
    int fd = open("/dev/adder", O_RDWR);
    //while(1) {
    for (i = 0; i < 10; i++) {
	sprintf(num, "%d", i);
        write(fd, num, strlen(num));
    }
    sleep(1);
    //}
    while(1) {
        ssize_t size = read(fd, &bytes, 64);
        printf("Read string: ");
        for (i = 0; i < size; i++) {
            printf("%c", bytes[i]);
        }
        printf("\n");
        sleep(1);
    }
    return 0;
}
