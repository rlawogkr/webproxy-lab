#include "../csapp.h"

int main(int argc, char** argv){ //argc: 입력받은 인자의 수, argv: 입력받은 인자들의 배열
    int clientfd;
    char* host, *port, buf[MAXLINE];
    rio_t rio; //rio_t 구조체 선언

    if(argc != 3){ // 인자의 수가 3개가 아니면
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]); // 에러 메시지 출력
        exit(0); // 프로그램 종료
    }

    host = argv[1];//전달받은 2번째 인자는 host
    port = argv[2];//전달받은 3번째 인자는 port

    clientfd = Open_clientfd(host, port); //host와 port를 이용하여 클라이언트 소켓을 생성
                                        //return 받은 소켓 식별자를 clientfd에 저장
    Rio_readinitb(&rio, clientfd); //rio 구조체 초기화

    while(Fgets(buf, MAXLINE, stdin) != NULL){ //표준 입력으로부터 한 줄씩 읽어서 buf에 저장
        Rio_writen(clientfd, buf, strlen(buf)); //buf에 저장된 내용을 clientfd에 쓰기
        Rio_readlineb(&rio, buf, MAXLINE); //clientfd로부터 한 줄씩 읽어서 buf에 저장
        Fputs(buf, stdout); //buf에 저장된 내용을 표준 출력으로 출력
    }

    Close(clientfd); //clientfd 소켓 닫기
    exit(0); //프로그램 종료

    
}