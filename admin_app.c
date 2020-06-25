#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFSIZE         512
#define MAX_NAMELEN     32
#define MAX_STUDENTS    9

#define SEAT_PORT       9001
#define MOTOR_PORT      9002
#define ENTRANCE_PORT   9003

typedef struct {
    int command;
    int time;
} MOTOR_DATA;

typedef struct {
    int student_number;
    float temp;
    int name_len;
    char name[MAX_NAMELEN];
    int seat_number;
} STUDENT_DATA;

typedef struct {
    int seat_number;
    int status;
} SEAT_DATA;

STUDENT_DATA students[MAX_STUDENTS];
int student_count = 0;

int SEAT_MAP[3][3] = {
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0}
};

int isUpdated = 0;

void *motor_client() {
    int sock;
    struct sockaddr_in    server_addr;
    char buf[BUFSIZE + 1];
    int retval, msglen, offset;

    MOTOR_DATA motor_data;

    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        perror("socket() error!\n");
        exit(0);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("192.168.1.3");
    server_addr.sin_port = htons(MOTOR_PORT);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        printf("[창문 서버와 연결이 실패하였습니다.]\n");
        close(sock);
        return (void *)0;
        perror("connect() error\n");
    }

    printf("[창문 서버와 연결되었습니다.]\n");
    printf("[다음 명령어로 창문을 제어하세요.]\n");
    
    while(1) {
        motor_data.command = 0;
        motor_data.time = 1;
        
        printf("[0: 열기, 1: 닫기, 3 (시간): 시간만큼 열기, -1: 서버 종료]\n");
        
        scanf("%d", &motor_data.command);
        if(motor_data.command == 3) {
            printf("[시간을 입력하세요 : (1~50)]\n");
            scanf("%d", &motor_data.time);
        }

        memset(buf, 0, sizeof(buf));
        
        offset = 0;

        memcpy(buf + offset, &(motor_data.command), sizeof(int));
        offset += sizeof(int);

        memcpy(buf + offset, &(motor_data.time), sizeof(int));
        
        retval = send(sock, buf, sizeof(int) * 2, 0);
        if(retval < 0) {
            perror("send() error\n");
            break;
        }

        printf("[창문 클라이언트] 명령어를 서버로 보냈습니다.\n");

        msglen = recv(sock, buf, BUFSIZE, 0);
        if (msglen < 0) {
            perror("read() error\n");
            exit(0);
        }
        else if (msglen == 0)
            continue;

        offset = 0;
		    
        memcpy(&(motor_data.command), buf + offset, sizeof(int));
        offset += sizeof(int);

        memcpy(&(motor_data.time), buf + offset, sizeof(int));
            
        printf("[창문 서버] 명령을 실행했습니다.\n");
        printf("명령어 : %d 시간 : %d\n", motor_data.command, motor_data.time);
        
        if(motor_data.command == -1) {
            printf("[창문 클라이언트] 클라이언트 종료\n");
            break;
        }
    }
    close(sock);
    return (void *)0;
}

void *entrance_server() {
    int listen_sock;
    int client_sock;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    int addrlen, retval, msglen, offset;
    
    char buf[BUFSIZE + 1];
    int seat_count = 1;

    STUDENT_DATA student_buffer;

    listen_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock == -1) {
        perror("socket() error!\n");
        exit(0);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(ENTRANCE_PORT);

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
        
        printf("[TCP 입실 서버] 입실 클라이언트 %d와 연결되었습니다.\n", ntohs(client_addr.sin_port));
        
        while(1) {
            msglen = recv(client_sock, buf, BUFSIZE, 0);
            if (msglen < 0) {
                perror("read() error\n");
                exit(0);
            }
            else if (msglen == 0)
                continue;

            buf[msglen] = '\0';
            offset = 0;

            memcpy(&(student_buffer.student_number), buf + offset, sizeof(int));
            offset += sizeof(int);
        
            memcpy(&(student_buffer.temp), buf + offset, sizeof(float));
            offset += sizeof(float);

            memcpy(&(student_buffer.name_len), buf + offset, sizeof(int));
            offset += sizeof(int);

            memcpy(&(student_buffer.name), buf + offset, sizeof(char) * student_buffer.name_len + 1);

            printf("[입실 서버] 학생 정보를 받았습니다.\n");

            student_buffer.seat_number = seat_count;

            students[student_count++] = student_buffer;
            isUpdated = 1;
            seat_count += 2;
        }
        close(client_sock);
    }
    close(listen_sock);
    return (void *)0;
}

void *seat_server() {
    int listen_sock;
    int client_sock;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    int addrlen, retval, msglen, offset;
    
    char buf[BUFSIZE +1];

    SEAT_DATA seat_data;
    char* seat_status[] = {"비움", "착석"};

    listen_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock == -1) {
        perror("socket() error!\n");
        exit(0);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SEAT_PORT);

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
        
        printf("[TCP 좌석 서버] 좌석 클라이언트 %d와 연결되었습니다.\n", ntohs(client_addr.sin_port));
        
        while(1) {
            msglen = recv(client_sock, buf, BUFSIZE, 0);
            if (msglen < 0) {
                perror("read() error\n");
                exit(0);
            }
            else if (msglen == 0)
                continue;

            offset = 0;

            memcpy(&(seat_data.seat_number), buf + offset, sizeof(int));
            offset += sizeof(int);
        
            memcpy(&(seat_data.status), buf + offset, sizeof(int));

            printf("[좌석 클라이언트 %d] %d번 좌석의 상태가 %s으로 바뀌었습니다.\n", ntohs(client_addr.sin_port), seat_data.seat_number, seat_status[seat_data.status]);
            
            SEAT_MAP[(seat_data.seat_number - 1) / 3][(seat_data.seat_number - 1) % 3] = seat_data.status;
            
            isUpdated = 1;
        }
        close(client_sock);
    }
    close(listen_sock);
    return (void *)0;
}

int main(int argc, char* argv[]) {
    pthread_t entrance_t, seat_t, motor_t;
    int entrance_t_result, seat_t_result, motor_t_result;

    int isFirst = 1;
    char* seat_status[] = {"비움", "착석"};

    if (pthread_create(&entrance_t, NULL, entrance_server, NULL) < 0) {
        perror("pthread_create error\n");
        exit(0);
    }

    if (pthread_create(&seat_t, NULL, seat_server, NULL) < 0) {
        perror("pthread_create error\n");
        exit(0);
    }

    if (pthread_create(&motor_t, NULL, motor_client, NULL) < 0) {
        perror("pthread_create error\n");
        exit(0);
    }

    while(1) {
        if(isUpdated || isFirst) {
            printf("\n좌석 상태\n");
            printf("[%s] [%s] [%s]\n", seat_status[SEAT_MAP[0][0]], seat_status[SEAT_MAP[0][1]], seat_status[SEAT_MAP[0][2]]);
            printf("[%s] [%s] [%s]\n", seat_status[SEAT_MAP[1][0]], seat_status[SEAT_MAP[1][1]], seat_status[SEAT_MAP[1][2]]);
            printf("[%s] [%s] [%s]\n\n", seat_status[SEAT_MAP[2][0]], seat_status[SEAT_MAP[2][1]], seat_status[SEAT_MAP[2][2]]);

            for(int i = 0; i<student_count; i++) {
                printf("[%d번 좌석] 학번 : %d, 온도 : %f, 이름 길이 : %d, 이름 : %s\n", students[i].seat_number, students[i].student_number, students[i].temp, students[i].name_len, students[i].name);
            }
            
            isUpdated = 0;
            isFirst = 0;
        }
        sleep(5);
    }

    pthread_join(entrance_t, (void **)&entrance_t_result);
    printf("Thread End %d\n", entrance_t_result);
    
    pthread_join(seat_t, (void **)&seat_t_result);
    printf("Thread End %d\n", seat_t_result);

    pthread_join(motor_t, (void **)&motor_t_result);
    printf("Thread End %d\n", motor_t_result);

    return 0;
} 

 