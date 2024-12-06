#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

void *send_msg(void *arg);
void *recv_msg(void *arg);

void error_handling(char *message);

char name[NAME_SIZE];
char msg[BUF_SIZE];

int main(int argc, char *argv[]){
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t t_snd, t_rcv;
    char user[NAME_SIZE];
    void *thread_return;

    if(argc!=4){
        printf("Usage : %s IP PORT NAME \n", argv[0]);
        exit(1);
    }

	sprintf(name, "[%s]", argv[3]);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0x00, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
        error_handling("connect() error");

    pthread_create(&t_snd, NULL, send_msg, (void*)&sock);
    pthread_create(&t_rcv, NULL, recv_msg, (void*)&sock);
    pthread_join(t_snd, &thread_return);
    pthread_join(t_rcv, &thread_return);
    close(sock);
    return 0;
}

//메시지 보낼때
void *send_msg(void *arg){
    int sock = *((int*)arg);
    char name_msg[NAME_SIZE+BUF_SIZE];
    
    while(1){
		printf("\r>> ");
		fflush(stdout);
        fgets(msg, BUF_SIZE, stdin);
        if(!strcmp(msg, "q\n")||!strcmp(msg, "Q\n")){
            close(sock);
            exit(0);
        }
        sprintf(name_msg, "%s %s", name, msg);
        write(sock, name_msg, strlen(name_msg));
    }
    return NULL;
}

//메시지 받을때
void *recv_msg(void *arg){
    int sock = *((int*)arg);
    char name_msg[NAME_SIZE+BUF_SIZE];
    int str_len;

    while(1){
        str_len = read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);
        if(str_len==-1){
            return (void*)-1;
        }
        name_msg[str_len] = 0;
        printf("\r%s\n", name_msg);
		printf(">> ");
		fflush(stdout);
    }
    return NULL;
}


void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
