#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MAXLINE                     511
#define PORT                        65021
#define ENTRANCE_PORT               9003

#define LED_MAJOR_NUMBER            501
#define LED_MINOR_NUMBER            101
#define LED_PATH_NAME               "/dev/led"

#define ULTRASONIC_MAJOR_NUMBER     504
#define ULTRASONIC_MINOR_NUMBER     104
#define ULTRASONIC_PATH_NAME        "/dev/ultrasonic"

#define INFRARED_THERMOPILE_MAJOR_NUMBER     502
#define INFRARED_THERMOPILE_MINOR_NUMBER     102
#define INFRARED_THERMOPILE_PATH_NAME        "/dev/infrared_thermopile"

#define MAX_LEN             32
#define MAX_NAMELEN         32
#define BUFSIZE             512

#define MAX_STUDENT_COUNT   60

typedef struct {
    int student_number;
    float temp;
    int name_len;
    char name[MAX_NAMELEN];
} STUDENT_DATA;

STUDENT_DATA student_data;
int isNewStudent = 0;

void *entrance_client() {
    int sock;
    struct sockaddr_in    server_addr;
    char buf[BUFSIZE + 1];
    int retval, msglen, offset;

    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        perror("socket() error!\n");
        exit(0);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("192.168.1.5");
    server_addr.sin_port = htons(ENTRANCE_PORT);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        printf("[관리자 서버와 연결이 실패하였습니다.]\n");
        close(sock);
        return (void *)0;
        perror("connect() error\n");
    }

    printf("[관리자 서버와 연결되었습니다.]\n");
    
    while(1) {
        if(isNewStudent) {
            memset(buf, 0, sizeof(buf));
            
            offset = 0;

            memcpy(buf + offset, &(student_data.student_number), sizeof(int));
            offset += sizeof(int);

            memcpy(buf + offset, &(student_data.temp), sizeof(float));
            offset += sizeof(float);

            memcpy(buf + offset, &(student_data.name_len), sizeof(int));
            offset += sizeof(float);
            
            memcpy(buf + offset, &(student_data.name), sizeof(char) * student_data.name_len);
            
            retval = send(sock, buf, sizeof(int) * 2 + sizeof(float) + sizeof(char) * student_data.name_len, 0);
            if(retval < 0) {
                perror("send() error\n");
                break;
            }

            printf("[입구 클라이언트] 학생 정보를 서버로 보냈습니다.\n");
            printf("학번 : %d, 온도 : %f, 이름 길이 : %d, 이름 : %s\n", student_data.student_number, student_data.temp, student_data.name_len, student_data.name);
            
            isNewStudent = 0;
        }       
        sleep(5);
    }
    close(sock);
    return (void *)0;
}

int main(int argc, char* argv[]) {
    char student[MAX_STUDENT_COUNT][29] ={0,};

    dev_t infrared_thermopile;
    int inf_fd;
    char temp_buf[MAX_LEN] = {0,};

    dev_t led;
    int led_fd;
    int led_status = 0;

    dev_t ultrasonic;
    int ultrasonic_fd;
    unsigned long distance = 0;

    pthread_t entrance_t;
    int entrance_t_result;

    infrared_thermopile = makedev(INFRARED_THERMOPILE_MAJOR_NUMBER, INFRARED_THERMOPILE_MINOR_NUMBER);
    mknod(INFRARED_THERMOPILE_PATH_NAME, S_IFCHR|0666, infrared_thermopile);

    led = makedev(LED_MAJOR_NUMBER, LED_MINOR_NUMBER);
    mknod(LED_PATH_NAME, S_IFCHR|0666, led);

    ultrasonic = makedev(ULTRASONIC_MAJOR_NUMBER, ULTRASONIC_MINOR_NUMBER);
    mknod(ULTRASONIC_PATH_NAME, S_IFCHR|0666, ultrasonic);

    inf_fd = open(INFRARED_THERMOPILE_PATH_NAME, O_RDONLY);
    led_fd = open(LED_PATH_NAME, O_RDWR);
    ultrasonic_fd = open(ULTRASONIC_PATH_NAME, O_RDONLY);

    if(inf_fd < 0) {
        printf("fail to open infrared_thermopile\n");
        return -1;
    }

    if(led_fd < 0) {
        printf("fail to open LED\n");
        return -1;
    }

    if(ultrasonic_fd < 0) {
        printf("fail to open ultrasonic\n");
        return -1;
    }

    if (pthread_create(&entrance_t, NULL, entrance_client, NULL) < 0) {
        perror("pthread create error\n");
        exit(0);
    }

    int flag =0;
    int count=1;
    int i = 0;

    int co =0;

    int cli_sock;

    struct sockaddr_in    serv_addr;

    int datalen;

    char buf[MAXLINE +1];
    int nbytes;

    char* addrserv;
    int nport;

    pid_t pid;

    struct hostent *host_ent;

    if (argc == 2) {
        nport = PORT;
    } else if (argc == 3) {
        addrserv =argv[1];
        nport = atoi(argv[2]);
    } else {
        printf("Usage: %s <server address>\n", argv[0]);
        printf("   or\nUsage: %s <server address <port>>\n", argv[0]);
        exit(0);
    }

    cli_sock = socket(PF_INET, SOCK_STREAM, 0);    // open socket
    if (cli_sock == -1) {
        perror("socket() error!\n");
        exit(0);
    }

    // addrserv: abc.der.com  type   and 192.168.0.xx type
    if (strncmp(addrserv, "192.168.", 8) != 0) {
        if ((host_ent = gethostbyname(addrserv)) == NULL) {
            herror("gethostbyname() error\n");
        }
        //sprintf(addrserv,"%s\n", ((struct in_addr *)host_ent->h_addr_list[0]));
        addrserv = inet_ntoa(*((struct in_addr *)host_ent->h_addr_list[0]));
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;                //default
    serv_addr.sin_addr.s_addr = inet_addr(addrserv);    //server address
    serv_addr.sin_port = htons(nport);            //server port to connect

    if (connect(cli_sock, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1) {    //connect to serve
        perror("connect() error\n");
        exit(0);
    }

    if((pid = fork()) == -1) {
        perror("fork() error\n");
        exit(0);
    } else if(pid ==0) {    //child process  : write data to server from stdin
        while(1) {
            fgets(buf, sizeof(buf), stdin);    //from stdin by user
            nbytes = strlen(buf);
            write(cli_sock, buf, MAXLINE);    //send user input string to socket -> client
            if(strncmp(buf, "exit", 4) ==0) {    //input: "exit" -> exit
                puts("exit program");    //output message
                exit(0);        //exit when "exit"
            }
        }
    } else if (pid > 0) {    //parent process : read data from server and output to stdout
        while(1) {  
            memset(buf, 0, sizeof(buf));
            if((nbytes = read(cli_sock, buf, MAXLINE)) < 0) {    //read string from client through socket
                perror("read() error\n");
                exit(0);
            }
            if(buf[1] == 0) 
                continue;

            for(int i=0;i<19;i++) {
                student[co][i] = buf[i];
            }

            while(1) { //디바이스드라이버 관련 알고리즘 작동부분 
                int max_temper =0;
                float real_max_temper =0;
                read(ultrasonic_fd, &distance, sizeof(unsigned long));
                
                if(distance < 20) { 
                    led_status = 1;
                    write(led_fd, &led_status, 4);
                    //초음파리드
                    read(inf_fd, &temp_buf, sizeof(char)*2);

                    int temp = (temp_buf[0] * 256 + (temp_buf[1] & 0xFC)) / 4;
                    float cTemp = temp * 0.03125;	//섭씨 ->측정
                    float fTemp = cTemp * 1.8 + 32; //화씨
                    if(max_temper < cTemp) {
                        max_temper = (int)cTemp;
                        real_max_temper = cTemp;
                        student_data.temp = real_max_temper;
                    }
                    // printf("Student Distance : %ld cm\n", (unsigned long)distance);
                    printf("--Now Checking...%d /5 sec--\n",count);
                    flag = 1;
                    count++; //5second on
                    sleep(1);	
                    if(count == 6 && flag == 1) {  //성공
                        for(int i =0 ; i <2 ; i++) {
                            printf("--Checking Complete--\n");
                            led_status = 0;
                            write(led_fd, &led_status, 4);
                            sleep(1);
                            led_status = 1;
                            write(led_fd, &led_status, 4);
                            sleep(1);
                        }
                        
                        printf("Student's temperature is %.2f C.\n",real_max_temper);

                        for(int i=19;i<23;i++) {
                            student[co][i] = max_temper>>(8*(22-i));
                        }
                        
                        printf("%d\n",student[co][0]);
                        printf("%X\n",student[co][1]);
                        printf("%X\n",student[co][2]);
                        printf("%X\n",student[co][3]);
                        printf("%X\n",student[co][4]);
                        
                        for(int j=5;j<15;j++) {
                            printf("%c",student[co][j]);
                            if(student[co][j] == NULL) {
                                student_data.name_len = j - 5;
                                strncpy(student_data.name, &student[co][5], student_data.name_len + 1);
                                break;
                            }
                        }
                        printf("\n");
                        printf("student number: %d .",(student[co][15]<<24) + (student[co][16]<<16) + (student[co][17]<<8)+(student[co][18]));
                        student_data.student_number = (student[co][15]<<24) + (student[co][16]<<16) + (student[co][17]<<8)+(student[co][18]);
                        printf("\n");
                        float real_temper = (student[co][19]<<24) + (student[co][20]<<16) +(student[co][21]<<8) +(student[co][22]);
                        printf("%f\n",real_temper);
                        co++;
                        buf[1] =0;
                        count =1;
                        flag = 0;
                        isNewStudent = 1;
                        break;
                    }
                }
                else if(count !=6 && flag == 1) {
                    flag = 0;
                    printf("--Student's Run , Please Rechecking--\n");
                    led_status=0;
                    write(led_fd, &led_status, 4);
                    break;
                }
                else {
                    count =1;
                    led_status=0;
                    write(led_fd, &led_status, 4);
                }
            }
            if(strncmp(buf, "exit",4) == 0)
                exit(0);
        } 
    }

    pthread_join(entrance_t, (void **)&entrance_t_result);
    printf("Thread End %d\n", entrance_t_result);

    close(cli_sock);
    close(inf_fd);
    close(led_fd);
    close(ultrasonic_fd);

    return 0;
}