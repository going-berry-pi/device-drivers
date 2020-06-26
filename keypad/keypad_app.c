#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define KEYPAD_MAJOR_NUMBER     505
#define KEYPAD_MINOR_NUMBER     105
#define KEYPAD_DEV_PATH_NAME    "/dev/keypad_dev"

int output0 =-1;
int output1 =-1;
int output2 =-1;
int output3 =-1;

int main(void)
{
    dev_t keypad_dev;
    int fd;
    int line;
    int index = -1;
    char characters[13] = {'0',};
    int result=0;
    keypad_dev = makedev(KEYPAD_MAJOR_NUMBER, KEYPAD_MINOR_NUMBER);
    mknod(KEYPAD_DEV_PATH_NAME, S_IFCHR|0666, keypad_dev);

    fd = open(KEYPAD_DEV_PATH_NAME, O_RDWR);

    if(fd < 0) {
        printf("fail to open keypad\n");
        return -1;
    }
    
    while(1) {
        line = 2;
        write(fd, &line, sizeof(int));
        read(fd, &index, sizeof(int));
        
        if(index != -1) {
            if(index !=output0){
                characters[0] = '1';
                characters[1] = '2';
                characters[2] = '3';
                printf("line 2 : %c\n", characters[index]);
                result = result*10+(characters[index]-'0');
                printf("%d\n",result);
                output0 = index;
            }
        }
        
        sleep(0.5);

        line = 7;
        write(fd, &line, sizeof(int));
        read(fd, &index, sizeof(int));
        if(index != -1) {
            if(index !=output1){
                characters[3] = '4';
                characters[4] = '5';
                characters[5] = '6';
                printf("line 7 :%c\n", characters[index+3]);
                result = result*10+(characters[index+3]-'0');
                printf("%d\n",result);
                output1 = index;
            }
        }

        sleep(0.5);
        
        line = 6;
        write(fd, &line, sizeof(int));
        read(fd, &index, sizeof(int));
        if(index != -1) {
            if(index !=output2){
                characters[6] = '7';
                characters[7] = '8';
                characters[8] = '9';
                printf("line 6 : %c\n", characters[index+6]);
                result = result*10+(characters[index+6]-'0');
                printf("%d\n",result);
                output2 = index;
            }
        }

        sleep(0.5);

        line = 4;
        write(fd, &line, sizeof(int));
        read(fd, &index, sizeof(int));
        if(index != -1) {
             if(index !=output3){
                characters[9] = '*';
                characters[10] = '0';
                characters[11] = '#';
                printf("line 4 : %c\n", characters[index+9]);
                result = result*10+(characters[index+9]-'0');
                printf("%d\n",result);
                output3 = index;
            }
        }
        
        sleep(0.5);
    }

    close(fd);

    return 0;
}

