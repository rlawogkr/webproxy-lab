/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd); // HTTP 트랜잭션을 처리하는 함수
void read_requesthdrs(rio_t *rp); // 요청 헤더를 읽고 무시하는 함수
int parse_uri(char *uri, char *filename, char *cgiargs); // URI를 분석하는 함수
void serve_static(int fd, char *filename, int filesize, char *method, char *version); // 정적 컨텐츠를 클라이언트에게 제공하는 함수
void get_filetype(char *filename, char *filetype); // 파일 이름의 접미어를 검사하여 파일 타입을 결정하는 함수
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method, char *version); // 동적 컨텐츠를 클라이언트에게 제공하는 함수
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg, char *version); // 에러 메세지를 클라이언트에게 보내는 함수

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {                // 받아야할 인자가 모두 들어오지 않은 경우
    fprintf(stderr, "usage: %s <port>\n", argv[0]);       // 이용법을 알려주고
    exit(1);              // 프로그램 종료
  }

  listenfd = Open_listenfd(argv[1]);                  // server쪽의 소켓을 선언해줌
                                                      // Open_listenfd : getaddrinfo로 소켓 생성에 사용할 정보 생성, socket으로 소켓의 디스크립터 생성,
                                                      // bind로 소켓의 주소와 디스크립터 결합, listen으로 server쪽에서 사용할 소켓이라는 것을 선언
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,                  // 클라이언트와 서버 소켓 연결
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,       // 소켓 주소 구조체(clientaddr)를 통해 호스트와 포트의 이름을 string으로 반환
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

void doit(int fd) {         // 한 개의 HTTP 트랜잭션을 처리하는 함수
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and hearders */
  Rio_readinitb(&rio, fd);                        // 식별자 fd를 rio_t 타입의 읽기 버퍼 rio와 연결
  Rio_readlineb(&rio, buf, MAXLINE);              // 다음 텍스트 줄을 버퍼 rio에서 읽고(종료 새 줄 문자(엔터)를 포함), 이를 buf로 복사하고, 텍스트 라인을 NULL 문자로 종료
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);      // 버퍼에서 포멧을 지정하여 읽어오는 함수
  // 요청 라인을 읽고 분석하는 부분

  // if(strcasecmp(method, "GET")) {                           // 만약 GET 이외의 요청을 했다면   // 원래 코드(GET 요청만 처리)
  if(strcasecmp(method, "GET") && strcasecmp(method, "HEAD")) {       // 만약 GET 또는 HEAD 이외의 요청을 했다면
    clienterror(fd, method, "501", "Not implemented",       // 에러 메세지 출력 후(tiny는 GET 요청만 처리 가능하므로)
                "Tiny does not implement this method", version);
    return;                                                 // main으로 리턴
  }

  read_requesthdrs(&rio);                 // 요청 헤더 읽기

  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs);          // uri를 파일 이름(filename)과 cgi 인자 스트링(비어있을 수 있음)으로 분석
  if(stat(filename, &sbuf) < 0) {                         // 파일 이름에 해당하는 파일이 디스크 상에 존재하지 않으면
    clienterror(fd, filename, "404", "Not found",         // 에러 메세지 출력후
                "Tiny couldn't find this file", version);
    return;                                               // main으로 리턴
  }

  if(is_static) { /* Serve static content */                            // 요청이 정적 컨텐츠를 위한 것이라면
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {         // 요청 받은 파일이 보통 파일인지, 읽기 권한을 가지고 있는 지 확인
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read the file", version);
      return;
    }
    serve_static(fd, filename, sbuf.st_size, method, version);                           // 정적 컨텐츠를 클라이언트에게 제공하는 함수
  }
  else {  /* Serve dynamic content */                                   // 요청이 동적 컨텐츠를 위한 것이라면
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {         // 요청 받은 파일이 보통 파일인지, 실행 가능한 파일인지 확인
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program", version);
      return;
    }
    serve_dynamic(fd, filename, cgiargs, method, version);                               // 동적 컨텐츠를 클라이언트에게 제공하는 함수
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg, char *version) {    // 에러 메세지를 클라이언트에게 보내는 함수
  char buf[MAXLINE], body[MAXLINE];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response*/
  sprintf(buf, "%s %s %s\r\n", version, errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp) {            // 요청 헤더를 읽고 무시하는 함수(Tiny 서버는 요청 헤더 내에 어떤 정보도 사용하지 않음)
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

// Tiny는 정적 컨텐츠를 위한 홈 디렉터리가 자신의 현재 디렉토리이고, 실행 파일의 홈 디렉토리는 /cgi-bin이라고 가정한다.
// 스트링 cgi-bin을 포함하는 모든 URI는 동적 컨텐츠를 요청하는 것으로 가정
// 기본 파일이름 : ./home.html
int parse_uri(char *uri, char *filename, char *cgiargs) {           
  char *ptr;
  // strstr : 두번째 인자 string이 첫번째 인자 string에 나오는지 확인(나오면 첫 번째에서 두 번째 인자가 시작하는 위치의 포인터 리턴, 없으면 NULL 리턴)
  if(!strstr(uri, "cgi-bin")) {     /* Static content */        // CGI 인자 스트링을 분석해서 정적 컨텐츠를 위한 것이라는 결과가 나오면
    strcpy(cgiargs, "");                  // CGI 인자 스트링을 지우고
    strcpy(filename, ".");
    strcat(filename, uri);                // URI를 ./index.html과 같은 경로로 바꿈
    if(uri[strlen(uri) - 1] == '/')       // 만약 URI가 / 문자로 끝나면
      strcat(filename, "home.html");      // 기본 파일 이름을 추가
    return 1;
  }
  else {      /* Dynamic content */         // 동적 컨텐츠를 위한 것이라면
    ptr = index(uri, '?');
    if(ptr) {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");                  // CGI 인자들을 모두 추출하고
    strcpy(filename, ".");
    strcat(filename, uri);                  // 나머지 URI를 상대 리눅스 파일 이름으로 변환
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize, char *method, char *version) {               // HEAD를 처리해주기 위해 char *method를 추가로 받음
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];
  
  /* Send response headers to client */
  get_filetype(filename, filetype);           // 파일 이름의 접미어를 검사하여 파일 타입을 결정
  sprintf(buf, "%s 200 OK\r\n", version);
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);      // 클라이언트에게 응답 줄(response line)과
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);                          // 응답 헤더를 보냄

  if(!strcasecmp(method, "HEAD"))               // HEAD 메소드로 요청을 받으면 헤더만 보내주고 끝 (11.11)
    return;

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0);        // 파일을 읽기 위해 filename을 오픈하고 식별자를 얻어옴
  /* 이전 코드 */
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);       // 요청한 파일을 가상 메모리 영역으로 매핑(srcfd의 첫 번째 filesize 바이트를 주소 srcp에서 시작하도록)
  // Close(srcfd);           // 파일을 메모리로 매핑했으므로 파일을 닫음
  // Rio_writen(fd, srcp, filesize);       // 파일을 클라이언트에게 전달(srcp에서 시작하는 filesize 만큼의 바이트를 클라이언트의 연결 식별자로 복사)
  // Munmap(srcp, filesize);               // 매핑된 가상 메모리 주소를 반환(free) => 메모리 누수 방지

  /* malloc 변경 코드 */            // 11.9
  srcp = (char *)malloc(filesize);
  Rio_readn(srcfd, srcp, filesize);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  free(srcp);
}

/*
 * get_filetype - Derive file type from filename
*/
void get_filetype(char *filename, char *filetype) {
  if(strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if(strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if(strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if(strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else if(strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if(strstr(filename, ".mpg"))
    strcpy(filetype, "video/mpeg");
  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs, char *method, char *version) {           // HEAD 요청을 처리해주기 위해 method를 추가로 받음
  char buf[MAXLINE], *emptylist[] = { NULL };

  /* Return first part of HTTP response */
  sprintf(buf, "%s 200 OK\r\n", version);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));                 // 성공을 알려주는 응답 라인을 보내는 것으로 시작(에러가 나는 것을 염두해 두지 않음)

  if(!strcasecmp(method, "HEAD"))          // 만약 HEAD 요청이 오면 헤더만 보내주고 끝 (11.11)
    return;

  if(Fork() == 0) {     /* Child */                 // 새로운 자식 프로세스를 fork를 통해 생성
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);                               // QUERY_STRING 환경 변수를 요청 URI의 CGI 인자들로 초기화
    Dup2(fd, STDOUT_FILENO);          /* Redirect stdout to client */       // 자식의 표준 출력을 연결된 파일의 식별자(client에게 보내는 거)로 재지정함
    Execve(filename, emptylist, environ);   /* Run CGI program */       // CGI 프로그램을 로드 후 실행
  }
  Wait(NULL);     /* Parent waits for and reaps child */          // 부모는 자식이 끝날 때까지 기다림
}