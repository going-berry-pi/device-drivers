#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#define RFID_MAJOR_NUMBER 503
#define RFID_MINOR_NUMBER 103
#define RFID_DEV_PATH_NAME "/dev/rfid"
#define MAX_STUDENT_COUNT 60
#define MAXLINE    511     // 데이터 크기
#define PORT    65021       // 임의로 설정한 통신포트

int stored_student_arr_int[MAX_STUDENT_COUNT] = {-1824295955, -2064293198,-1904510466,894389076};
char stored_student_arr_char[MAX_STUDENT_COUNT][10]={"HyoSung","YeonHo","JuHyeon","ByeongJun"};

int class_in_student_arr_int[MAX_STUDENT_COUNT] = {0,};
int uid_stored_student_count = 4;
int class_in_student_count = 0;
char student_arr[MAX_STUDENT_COUNT][15] = {0,};
char *uid_to_name(int uid){
	for(int i = 0; i<MAX_STUDENT_COUNT; i++){
		if(stored_student_arr_int[i]==uid){
			return stored_student_arr_char[i];
		}
	}
	return 0;
}

void delay(int ms) {
#ifdef WIN32
	Sleep(ms);
#else
	usleep(ms * 1000);
#endif
}
int main(int argc, char* argv[]){
    
	dev_t rfid_dev;

	rfid_dev = makedev(RFID_MAJOR_NUMBER, RFID_MINOR_NUMBER);
	mknod(RFID_DEV_PATH_NAME, S_IFCHR | 666, rfid_dev);
	int fd_rfid_dev = open("/dev/rfid", O_RDWR);
	if (fd_rfid_dev == -1) {
		perror("failed to open rfid_dev; because ");
		return 1;
	}
    
	char new_name[10];
    int serv_sock;
    int conn_sock;

    struct sockaddr_in    serv_addr;
    struct sockaddr_in    conn_addr;

    int addrlen, datalen;

    char buf[512];
    int nbytes;
    int nport;

    pid_t pid;

    if (argc == 1){
        nport = PORT;    //default port : 65021
    } else if (argc == 2) {
        nport = atoi(argv[1]);
    } else {
        printf("Usage: %s <port>\n", argv[0]);
        exit(0);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv_sock == -1){
        perror("socket() error!\n");
        exit(0);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;            //default
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);     //my addr
    serv_addr.sin_port = htons(nport);        //my port to serv

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind() error\n");
        exit(0);
    }

    if(listen(serv_sock, 1) == -1) {
        perror("listen() error\n");
        exit(0);
    }

    addrlen = sizeof(conn_sock);    // connection socket
    conn_sock = accept(serv_sock, (struct sockaddr *)&conn_addr, &addrlen); //wait for call
    if(conn_sock == -1){
        perror("accept() error\n");
        exit(0);
    }

    if((pid = fork()) == -1){
        close(conn_sock);
        perror("fork() error\n");
    } else if(pid ==0) {    //child process
        while(1){
            int int_uid = 0;
			// Look for a card

			read(fd_rfid_dev, &int_uid, 4);

			// Print UID
			if(int_uid != 0){
					for(int i = 0; i<class_in_student_count; i++){
						if((student_arr[i][1]<<24)+(student_arr[i][2]<<16)+(student_arr[i][3]<<8)+student_arr[i][4] == int_uid){
							printf("%s already exists in class.\n", uid_to_name(int_uid));
							int_uid = 0;
							break;
						}
					}
				if(int_uid == 0){
					continue;
				}
				if(uid_to_name(int_uid) == 0){
					printf("This Card need to sign in\n");
					printf("What your name? : \n");
					scanf("%s", new_name);
					strcpy(stored_student_arr_char[uid_stored_student_count], new_name);
					stored_student_arr_int[uid_stored_student_count] = int_uid;
					uid_stored_student_count++;
				}
				student_arr[class_in_student_count][1] = int_uid>>24;
				student_arr[class_in_student_count][2] = int_uid>>16;
				student_arr[class_in_student_count][3] = int_uid>>8;
				student_arr[class_in_student_count][4] = int_uid;
				student_arr[class_in_student_count][0] = class_in_student_count;
				for(int i = 5; i<15; i++){
					student_arr[class_in_student_count][i] = uid_to_name(int_uid)[i-5];
					if(uid_to_name(int_uid)[i-5] == NULL){
						break;
					}
				}
				
				printf("New Student : ");
				for(int i =5; i<15; i++){
				printf("%c", uid_to_name(int_uid)[i-5]);
					if(uid_to_name(int_uid)[i-5]==NULL){
						break;
					}
				}
				
            nbytes = 15;
            printf("\n%d\n", class_in_student_count);
			
            write(conn_sock, student_arr[class_in_student_count], MAXLINE);    //입력된 문자를 소켓을 통해 전송
            if(strncmp(student_arr[class_in_student_count], "exit", 4) ==0) {        //입력 문자가 "exit" 로 시작하면 -> exit 콜 =>종료
                puts("exit program");    //output message
                exit(0);        //exit when "exit"
            }
            
				class_in_student_count++;
				printf("\n Current Class Students(%d students) : \n", class_in_student_count);
				for(int i =0; i<MAX_STUDENT_COUNT; i++){
					if(student_arr[i][1]==0){
						break;
					}
					for(int j =5; j<15;j++){
					printf("%c", student_arr[i][j]);
					if(student_arr[i][j]==NULL){
						break;
					}
					}
					printf("\n");
				}
			}
			
			delay(1000);
        }
    } else if (pid > 0){    //parent process
        while(1){
            if((nbytes = read(conn_sock, buf, MAXLINE)) < 0){    // 클라이언트에서 온 문자를 읽어
                perror("read() error\n");
                exit(0);
            }
            printf("%s\n", buf);    // 표준 출력으로 출력
            if(strncmp(buf, "exit",4) == 0)
                exit(0);
        }
    }

    close(conn_sock);
    close(serv_sock);
	close(fd_rfid_dev);
    return 0;
} 