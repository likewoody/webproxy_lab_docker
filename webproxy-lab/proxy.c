#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void doit(int fd);
void read_requesthdrs(rio_t *rp, int serverfd);
void parse_uri(char *uri, char *hostname, char *port, char *path);
void *thread(void *vargp);

int main(int argc, char **argv)
{
  int listenfd;
  char hostname[MAXLINE], port[MAXLINE]; /* MAXLINE - 8192 */
  socklen_t clientlen; /* __U32_TYPE */
  struct sockaddr_storage clientaddr; /* 소켓 주소 저장소 */
  pthread_t tid;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); /* argv[1] - 포트 */

  while (1)
  {
    clientlen = sizeof(clientaddr); /* sizeof(sockaddr_storage) */
    int *connfdp = malloc(sizeof(int)); /* 스레드 간 공유 방지 */
    *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen); // line:netp:tiny:accept
    /* 소켓 주소 구조체 -> 스트링 */
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, 
                MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);

    Pthread_create(&tid, NULL, thread, connfdp);
  }
}

/*
    - 에러 메시지를 읽고 클라이언트에게 보냄
*/
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg)
{
    char buf[MAXLINE], body[MAXLINE];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void doit(int fd)
{
  struct stat sbuf; /* 파일의 메타 데이터 정보를 담고 있음 */
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], hostname[MAXLINE], port[MAXLINE], path[MAXLINE];
  int serverfd, content_length, *response_ptr;
  /* request와 response 분리 */
  rio_t request_rio, response_rio;

  /* 1. Client -> Proxy */
  Rio_readinitb(&request_rio, fd);            /* request_rio 초기화 with fd */
  Rio_readlineb(&request_rio, buf, MAXLINE);  /* read from client to buf */
  printf("Request headers:\n %s\n", buf);     /* client로부터 읽어온 Request header 읽기 */

  sscanf(buf, "%s %s", method, uri);          /* buf 문자열에서 method, uri 변수로 저장 */
  parse_uri(uri, hostname, port, path);       /* uri에서 hostname, port, path 나누는 작업 */


  /* Server에 전송하기 위해 요청 라인의 형식 변겅: method path version -> GET / HTTP/1.0 */
  sprintf(buf, "%s %s %s\r\n", method, path, version);

  // 지원하지 않는 method인 경우 예외 처리
  if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD"))
  {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  
  serverfd = Open_clientfd(hostname, port); /* 프록시가 서버와 통신하기 위한 serverfd */
  if (serverfd < 0)
  {
    clienterror(serverfd, method, "502", "Bad Gateway", 
            "Failed to establish connection with the end server");
      return;
  }

  /* 2. Proxy -> Server */
  Rio_writen(serverfd, buf, strlen(buf));   /* Rquest line을 서버로 전송 (90번째 줄에서 만든 buf) */
  read_requesthdrs(&request_rio, serverfd); /* Read HTTP header & write */

  /* 3. Server -> Proxy (Response)*/
  Rio_readinitb(&response_rio, serverfd);
  /* Response header 읽기 & 전송 */
  while (strcmp(buf, "\r\n"))               
  {
    Rio_readlineb(&response_rio, buf, MAXLINE);
    if (strstr(buf, "Content-length")) /* Response Body 수신에 사용하기 위해 Content-length 저장 */
      content_length = atoi(strchr(buf, ':') + 1);
    Rio_writen(fd, buf, strlen(buf));
  }

  /* Response Body 읽기 & 전송 */
  response_ptr = malloc(content_length);
  Rio_readnb(&response_rio, response_ptr, content_length);
  Rio_writen(fd, response_ptr, content_length); /* Client에 Response Body 전송 */
  free(response_ptr); // 캐싱하지 않은 경우만 메모리 반환

  Close(serverfd);
}

/* 
  Request header를 읽고 Server에 전송하는 함수
  필수 헤더가 없는 경우에는 필수 헤더를 추가로 전송
*/
void read_requesthdrs(rio_t *rp, int serverfd)
{
    char buf[MAXLINE]; /* 8192 크기의 버퍼 생성 */
    while (Rio_readlineb(rp, buf, MAXLINE) > 0) {
        if (strcmp(buf, "\r\n") == 0) {
            Rio_writen(serverfd, buf, strlen(buf)); // 마지막 빈 줄도 포함
            break;
        }
        Rio_writen(serverfd, buf, strlen(buf)); // 서버에 전송
    }
}
void parse_uri(char *uri, char *hostname, char *port, char *path) {
    char *hostbegin, *hostend, *portptr, *pathptr;

    // "http://" 또는 "https://" 제거
    if ((hostbegin = strstr(uri, "//")) != NULL)
        hostbegin += 2; // "//" 뒤부터 hostname
    else
        hostbegin = uri;

    // path 시작 위치 (없으면 NULL → 기본 "/")
    pathptr = strchr(hostbegin, '/');
    if (pathptr) {
        strcpy(path, pathptr);  // path 저장
    } else {
        strcpy(path, "/");      // 기본 path
    }

    // 포트 위치 탐색
    portptr = strchr(hostbegin, ':');

    if (portptr && (!pathptr || portptr < pathptr)) {
        // 포트가 있고 path보다 앞에 있음
        *portptr = '\0';  // hostname 끝을 자름
        strcpy(hostname, hostbegin);
        portptr++; // ':' 다음 문자부터
        if (pathptr) {
            *pathptr = '\0'; // 포트 문자열 끝 자름
            strcpy(port, portptr);
            *pathptr = '/';  // 복원
        } else {
            strcpy(port, portptr);
        }
    } else {
        // 포트가 없음
        if (pathptr)
            hostend = pathptr;
        else
            hostend = hostbegin + strlen(hostbegin);

        strncpy(hostname, hostbegin, hostend - hostbegin);
        hostname[hostend - hostbegin] = '\0';
        strcpy(port, "80"); // 기본 포트
    }
}

void *thread(void *vargp) {
  int connfd = *((int *) vargp);
  Pthread_detach(pthread_self()); /* 현재 스레드 작업이 종료되면 메모리 자원이 시스템에 의해 자동으로 반환 */
  free(vargp);
  doit(connfd);
  Close(connfd);
}