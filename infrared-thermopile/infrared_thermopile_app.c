#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define INFRARED_THERMOPILE_MAJOR_NUMBER     502
#define INFRARED_THERMOPILE_MINOR_NUMBER     102
#define INFRARED_THERMOPILE_PATH_NAME        "/dev/infrared_thermopile"

#define MAX_LEN 32

int main(void)
{
    dev_t infrared_thermopile;
    int fd;
    unsigned long distance = 0;
    char buf[MAX_LEN] = {0,};

    infrared_thermopile = makedev(INFRARED_THERMOPILE_MAJOR_NUMBER, INFRARED_THERMOPILE_MINOR_NUMBER);
    mknod(INFRARED_THERMOPILE_PATH_NAME, S_IFCHR|0666, infrared_thermopile);

    fd = open(INFRARED_THERMOPILE_PATH_NAME, O_RDONLY);

    if(fd < 0) {
        printf("fail to open infrared_thermopile\n");
        return -1;
    }

    while(1) {
        read(fd, &buf, sizeof(char)*2);
        
        int temp = (buf[0] * 256 + (buf[1] & 0xFC)) / 4;
        float cTemp = temp * 0.03125;
        float fTemp = cTemp * 1.8 + 32;
        
        printf("Temperature in Celsius is : %.2f C \n", cTemp);
        printf("Temperature in Fahrenheit is : %.2f F \n", fTemp);
        
        sleep(1);
    }

    close(fd);

    return 0;
}
