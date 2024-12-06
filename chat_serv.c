#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

void *handle_clnt(void *arg); 
void send_msg(char *msg, int len);
void error_handling(char *message);


//스레드간 공유 변수
int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutex;

int main(int argc, char *argv[]){
    int serv_sock, cli_sock;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_addr_len;
    pthread_t t_id;

    if(argc!=2){
        printf("Usage: %s port\n", argv[0]);
        exit(1);
    }

    //뮤텍스 생성(초기화)
    pthread_mutex_init(&mutex, NULL);


    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_sock==-1)
        error_handling("socket() error");

    memset(&serv_addr, 0x00, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    
    if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
        error_handling("bind() error");
    
    if(listen(serv_sock, 5)==-1)
        error_handling("listen() error");
    
    
    while(1){
        cli_addr_len = sizeof(cli_addr);
        
        cli_sock = accept(serv_sock, (struct sockaddr*)&cli_addr, &cli_addr_len);
        
        pthread_mutex_lock(&mutex); //뮤텍스 획득
        clnt_socks[clnt_cnt++] = cli_sock; //클라이언트 소켓 배열에 참가한 클라이언트 소켓 추가
        pthread_mutex_unlock(&mutex); //뮤텍스 반환
        
        pthread_create(&t_id, NULL, handle_clnt, (void*)&cli_sock);
        pthread_detach(t_id); //블로킹 시키지 않고 진행 -> 완료되면 리소스 반환
        printf("Connected client IP: %s \n", inet_ntoa(cli_addr.sin_addr));
    }
    close(serv_sock);
    return 0;

}


//클라이언트 연결시 메시지 전송 & 연결 해제 관리 핸들러
void *handle_clnt(void *arg){
    int cli_sock = *((int*)arg);
    int str_len = 0;
    char msg[BUF_SIZE];

    while((str_len = read(cli_sock, msg, BUF_SIZE-1))>0)
        send_msg(msg, str_len);

    //while문이 끝났다는건 채팅을 종료했다는 뜻 -> 종료한 클라이언트 소켓을 삭제시켜야됨.(클라 소켓 배열에서)
    pthread_mutex_lock(&mutex);
    for(int i=0; i<clnt_cnt;i++){
        if(cli_sock==clnt_socks[i]){    //클라이언트 소켓 배열에서 종료 클라이언트 소켓을 찾으면
            while(i++<clnt_cnt-1)   //i번째부터 클라이언트 소켓 배열을 앞으로 하나씩 당긴다
                clnt_socks[i] = clnt_socks[i+1]; 
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutex);
    close(cli_sock); //cli_sock도 닫는다.
    return NULL;
}

//메시지 전송 to all client
void send_msg(char *msg, int len){
    
    pthread_mutex_lock(&mutex);
    for(int i=0; i<clnt_cnt;i++){
        write(clnt_socks[i], msg, len);
    }
    pthread_mutex_unlock(&mutex);
}

void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
