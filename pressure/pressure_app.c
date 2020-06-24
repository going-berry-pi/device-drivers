#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>
#include <time.h>

#define PRESSURE_MAJOR_NUMBER	506
#define PRESSURE_MINOR_NUMBER	106
#define PRESSURE_DEV_PATH_NAME	"/dev/pressure"

int time_limit=0;
static int seat_number =1;//seat number which is allocated to each pressure sensor.
int main(int argc, char ** argv){
	
	printf("Set time limit (minutes) : ");
	scanf("%d",&time_limit);
	printf("\nseat number is : %d\n",seat_number);
	
	sleep(1);
	dev_t pressure;
	int dev;
	printf("Pressure sensor activate\n");
	
	char str[100];
	char buffer[1024];
	pressure = makedev(PRESSURE_MAJOR_NUMBER,PRESSURE_MINOR_NUMBER);
	mknod(PRESSURE_DEV_PATH_NAME,S_IFCHR|0666,pressure);
	
	dev = open(PRESSURE_DEV_PATH_NAME,O_RDWR);
	if(dev <-1){
		perror("failed to open; because");
		return 1;
		}
	clock_t start = clock();
    unsigned int count =0;
	while(1){
			usleep(1000*1000);
			int current=0;
			read(dev, &current, sizeof(int));
		if(current<time_limit){
			count++;
			printf("count : %d , current pressure : %d\n",count,current);
		}
		if(current>time_limit){
			count =0;
			printf("count : %d , current pressure : %d\n",count,current);
		}
		if(count>time_limit){
			clock_t end = clock(); 
			printf("%d seconds after no pressure\n",count-time_limit);
			}
	}

	close(dev);
	exit(EXIT_SUCCESS);
	
	return 0;
}
