#include "csapp.h"

void echo(int connfd); //client와 통신하는 함수 echo 선언

int main(int argc, char **argv){
    int listenfd, connfd;//리스닝 소켓과 연결된 클라이언트 소켓의 fd
    socklen_t clientlen;//클라이언트 주소의 길이
    struct sockaddr_storage clientaddr;//클라이언트의 주소 정보를 저장할 구조체
    char clinet_hostname[MAXLINE], client_port[MAXLINE];//client의 host이름과 port번호를 저장할 배열

    if (argc != 2){//인자의 수가 2개가 아니면
        fprintf(stderr, "usage: %s <port>\n", argv[0]);//에러 메시지 출력
        exit(0);//프로그램 종료
    }

    listenfd = Open_listenfd(argv[1]);//전달받은 포트번호를 이용하여 리스닝 소켓 생성
    while(1){
        clientlen = sizeof(struct sockaddr_storage);//클라이언트 주소의 길이를 저장
        connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);//클라이언트의 연결 요청을 수락하고 연결된 소켓의 fd를 반환
        Getnameinfo((SA*)&clientaddr, clientlen, clinet_hostname, MAXLINE, client_port, MAXLINE, 0);//클라이언트의 주소 정보를 host이름과 port번호로 변환
        printf("Connected to (%s, %s)\n", clinet_hostname, client_port);//연결된 클라이언트의 host이름과 port번호 출력
        echo(connfd);//연결된 클라이언트와 통신하는 함수 호출
        Close(connfd);//연결된 클라이언트 소켓 닫기
    }
    exit(0);
}
void echo(int connfd){
    size_t n;//읽은 바이트 수
    char buf[MAXLINE];//버퍼
    rio_t rio;//rio 구조체 선언

    Rio_readinitb(&rio, connfd);//rio 구조체 초기화
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){//client로부터 한 줄씩 읽어서 buf에 저장
        printf("server received %d bytes\n", (int)n);//읽은 바이트 수 출력
        Rio_writen(connfd, buf, n);//읽은 내용을 client에게 다시 보내기
    }
}