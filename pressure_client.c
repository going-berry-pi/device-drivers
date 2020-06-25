#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#define PRESSURE_MAJOR_NUMBER 	506
#define PRESSURE_MINOR_NUMBER   106
#define PRESSURE_DEV_PATH_NAME  "/dev/pressure"

#define MINIMUM_PRESSURE        10

#define BUFSIZE    512

typedef struct {
    int seat_number;
    int status;
} SEAT_DATA;

int global_status = 0;

void *seat_client() {
    int sock;
    struct sockaddr_in    server_addr;
    char buf[BUFSIZE +1];
    int retval, msglen, offset;

    SEAT_DATA seat_data;
    seat_data.seat_number = 1;
    seat_data.status = 0;
    char* seat_status[] = {"비움", "착석"};

    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        perror("socket() error!\n");
        exit(0);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("192.168.1.5");
    server_addr.sin_port = htons(9001);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect() error\n");
        exit(0);
    }

    while(1) {
        if(seat_data.status != global_status) {
            seat_data.status = global_status;
            
            memset(buf, 0, sizeof(buf));
            
            offset = 0;
            
            memcpy(buf + offset, &(seat_data.seat_number), sizeof(int));
            offset += sizeof(int);

            memcpy(buf + offset, &(seat_data.status), sizeof(int));

            retval = send(sock, buf, sizeof(int) * 2, 0);
            if(retval < 0) {
                perror("send() error\n");
                break;
            }

            printf("[좌석 클라이언트] 좌석 상태를 서버로 보냈습니다.\n");
            printf("[좌석 클라이언트] 좌석 번호 : %d 좌석 상태 : %s\n", seat_data.seat_number, seat_status[seat_data.status]);
        }
        sleep(10);
    }

    close(sock);
    return (void *)0;
}
int main(int argc, char ** argv) {

    dev_t pressure;
    int dev;
    int cur_pressure = 0;
    int status = 0;
    char* seat_status[] = {"비움", "착석"};

    pthread_t thread_t;
    int thread_result;

    pressure = makedev(PRESSURE_MAJOR_NUMBER,PRESSURE_MINOR_NUMBER);
    mknod(PRESSURE_DEV_PATH_NAME,S_IFCHR|0666,pressure);

    dev = open(PRESSURE_DEV_PATH_NAME,O_RDONLY);
    if(dev <-1) {
        perror("failed to open");
        return 1;
    }

    if (pthread_create(&thread_t, NULL, seat_client, NULL) < 0) {
        perror("pthread_create error\n");
        exit(0);
    }

    while(1) {
        int count = 0;
        global_status = status;
        
        while(1) {
            read(dev, &cur_pressure, sizeof(int));

            if (status && (cur_pressure < MINIMUM_PRESSURE)) {
                count++;
            }
            
            if (!status && (cur_pressure > MINIMUM_PRESSURE)) {
                count++;
            }
            
            printf("좌석 상태 : %s\n", seat_status[status]);
            printf("현재 압력 : %d\n", cur_pressure);
            
            if(count > 10) {
                printf("좌석 상태 바뀜 : [%s -> %s]\n", seat_status[status], seat_status[!status]);
                status = !status;
                break;
            }
            
            sleep(1);
        }
    }

    pthread_join(thread_t, (void **)&thread_result);
    printf("Thread End %d\n", thread_result);

    close(dev);
    exit(EXIT_SUCCESS);

    return 0;
}
