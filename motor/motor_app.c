#include <unistd.h> 
#include <fcntl.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>

#include <sys/stat.h>
#include <sys/ioctl.h> 
#include <sys/types.h> 
#include <sys/sysmacros.h>  

#define MOTOR_MAJOR_NUMBER   507
#define MOTOR_MINOR_NUMBER 	107
#define MOTOR_DEV_NAME      "motor_dev"

#define MOTOR_PATH_NAME 		"/dev/motor_dev"

#define INTERVAL 		50000
int main (int argc, char ** argv ){ 

	dev_t motor;
	int motor_fd; 
	char buffer[1024];
	int cmd =1;
	
	printf("MORTOR Control program\n"); 
	
	motor  = makedev(MOTOR_MAJOR_NUMBER, MOTOR_MINOR_NUMBER);
	mknod(MOTOR_PATH_NAME, S_IFCHR|0666, motor);
	motor_fd = open(MOTOR_PATH_NAME, O_WRONLY);	
	
	if (motor_fd < 0)
	{ 
		printf("failed to open motor device\n"); 
	}

	printf("--If you terminate program : 3 ");
	int count;
	int Time;
	while(1)
	{	
		count =0;
		printf("\n--Input( OPEN : 0 /CLOSE : 1 / OPEN TimeMode : 3 / CLOSE AND END : -1) : ");
		scanf("%d", &cmd);
		if(cmd ==0){
			printf(" >>OPEN Window\n");
		}
		else if(cmd == 1){
			printf(" >>CLOSE Window\n");
		}
		
		else if(cmd == 3 ){ 
			printf(" Time (sec) : ");
			scanf("%d", &Time);
			printf(">>OPEN During %d second.\n",Time);
			while(count <= Time){
				printf(">>%d/%d sec\n",count,Time); 
				if(count ==Time){  
					cmd = 1;
					fflush(stdin);
					ioctl(motor_fd, cmd);	
				}
				count++;
				fflush(stdin);
				ioctl(motor_fd, cmd);
				sleep(1);
			}
		}
		else if(cmd ==-1){
			printf(" >>CLOSE Window AND OFF \n");
		}
		fflush(stdin);
		ioctl(motor_fd, cmd);
		sleep(1);
		if(cmd == -1){
		 	break;
		 }
	}
	close(motor_fd); 
}
