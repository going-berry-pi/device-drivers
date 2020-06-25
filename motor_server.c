#include <unistd.h> 
#include <fcntl.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>

#include <sys/stat.h>
#include <sys/ioctl.h> 
#include <sys/types.h> 
#include <sys/sysmacros.h>  
#include <arpa/inet.h>
#include <sys/socket.h>

#define MOTOR_MAJOR_NUMBER   507
#define MOTOR_MINOR_NUMBER   107
#define MOTOR_PATH_NAME      "/dev/motor_dev"

#define BUFSIZE              512

typedef struct {
    int command;
    int time;
} MOTOR_DATA;

int main (int argc, char ** argv ) { 
    dev_t motor;
    int fd; 

    int listen_sock;
    int client_sock;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    int addrlen, retval, msglen, offset;

    char buf[BUFSIZE + 1];

    MOTOR_DATA motor_data;

    motor = makedev(MOTOR_MAJOR_NUMBER, MOTOR_MINOR_NUMBER);
    mknod(MOTOR_PATH_NAME, S_IFCHR|0666, motor);
	
    fd = open(MOTOR_PATH_NAME, O_WRONLY);	
	
    if (fd < 0) { 
        printf("failed to open motor device\n"); 
        return -1;
    }

    listen_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock == -1) {
        perror("socket() error!\n");
        exit(0);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(9003);

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind() error\n");
        exit(0);
    }

    if (listen(listen_sock, 1) == -1) {
        perror("listen() error\n");
        exit(0);
    }

    while(1) {
        addrlen = sizeof(client_sock);
        client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addrlen);
        if (client_sock == -1) {
            perror("accept() error\n");
            exit(0);
        }
        
        printf("[TCP 모터 서버] 클라이언트 %d와 연결되었습니다.\n", ntohs(client_addr.sin_port));
        
        while(1) {
            msglen = recv(client_sock, buf, BUFSIZE, 0);
            if (msglen < 0) {
                perror("read() error\n");
                exit(0);
            }
            else if (msglen == 0)
                break;

            offset = 0;
            
            memcpy(&(motor_data.command), buf + offset, sizeof(int));
            offset += sizeof(int);

            memcpy(&(motor_data.time), buf + offset, sizeof(int));
            
            printf("명령어 : %d 시간 : %d\n", motor_data.command, motor_data.time);
            
            if (motor_data.command == 3) {
                int count = 0;
                ioctl(fd, motor_data.command);
                sleep(motor_data.time);
                ioctl(fd, 1);
            }
            else {
                ioctl(fd, motor_data.command);
            }

            retval = send(client_sock, buf, sizeof(int) * 2, 0);
            if(retval < 0) {
                perror("send() error\n");
                break;
            }
        }
        close(client_sock);
        printf("[TCP 모터 서버] 클라이언트 %d가 종료되었습니다.\n", ntohs(client_addr.sin_port));
        exit(0);
    }
    close(listen_sock);
    close(fd);

    return 0;
}
