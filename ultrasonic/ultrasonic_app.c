#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define ULTRASONIC_MAJOR_NUMBER     504
#define ULTRASONIC_MINOR_NUMBER     104
#define ULTRASONIC_PATH_NAME        "/dev/ultrasonic"

int main(void)
{
    dev_t ultrasonic;
    int fd;
    unsigned long distance = 0;

    ultrasonic = makedev(ULTRASONIC_MAJOR_NUMBER, ULTRASONIC_MINOR_NUMBER);
    mknod(ULTRASONIC_PATH_NAME, S_IFCHR|0666, ultrasonic);

    fd = open(ULTRASONIC_PATH_NAME, O_RDONLY);

    if(fd < 0) {
        printf("fail to open ultrasonic\n");
        return -1;
    }

    while(1) {
        read(fd, &distance, sizeof(unsigned long));
        printf("Distance : %ld cm\n", (unsigned long)distance);
        sleep(1);
    }

    close(fd);

    return 0;
}
