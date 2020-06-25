#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define LED_MAJOR_NUMBER     501
#define LED_MINOR_NUMBER     101
#define LED_PATH_NAME        "/dev/led"

int main(void)
{
	dev_t led;
	int fd;
	int led_status = 0;
	
	led = makedev(LED_MAJOR_NUMBER, LED_MINOR_NUMBER);
	mknod(LED_PATH_NAME, S_IFCHR|0666, led);
	
	fd = open(LED_PATH_NAME, O_RDWR);

	if(fd < 0) {
		printf("fail to open ultrasonic\n");
		return -1;
	}

	while(1) {
        led_status=1;
        write(fd, &led_status, 4);
        sleep(1);
        led_status=0;
        write(fd, &led_status, 4);
        sleep(1);
    }
  
    close(fd);

    return 0;
}
