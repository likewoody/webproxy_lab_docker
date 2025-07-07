/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *version);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *version);
void response_requesthdrs(int fd, char *buf, char *version, char *filename, int filesize);


int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE]; /* MAXLINE - 8192 */
  socklen_t clientlen; /* __U32_TYPE */
  struct sockaddr_storage clientaddr; /* 소켓 주소 저장소 */

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
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // line:netp:tiny:accept
    /* 소켓 주소 구조체 -> 스트링 */
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, 
                MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
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
  int is_static; /* Flag static or dynamic */
  struct stat sbuf; /* 파일의 메타 데이터 정보를 담고 있음 */
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  sprintf(buf, "Request headers:\r\n");

  /* Read request line and headers */
  Rio_readinitb(&rio, fd); /* rio에 fd를 등록하고 다른 필드들을 초기화 */
  Rio_readlineb(&rio, buf, MAXLINE); /* buf에 있는 데이터를 rio에 담음 */
  
  sprintf(buf, "%s\r\n", buf); 
  /* 하나의 긴 문자열 buf를 %s %s %s 포맷 형식으로 나눈다 */
  sscanf(buf, "%s %s %s", method, uri, version); 

  /* telnet HEAD 접근할 경우 */
  if (!strcasecmp(method, "HEAD")) {
    response_requesthdrs(fd, buf, version, filename, sbuf.st_size);
    return;  
  }
  if (strcasecmp(method, "GET")) { /* 대소문자 상관없이 get, GET인지 비교 */
    /* 같을 때 0, 그러므로 같지 않을 때 에러 발생 */
    clienterror(fd, method, "501", "Not implemented",
            "Tiny does not implement this method");
    return;
  }

  read_requesthdrs(&rio); /* Read HTTP header */

  /* Parser URI from GET reqeust */
  is_static = parse_uri(uri, filename, cgiargs);
  if (stat(filename, &sbuf) < 0) { /* stat() 함수가 파일 또는 디렉토리의 메타데이터 정보를 얻는 함수 */
    /* 성공하면 0, 실패하면 -1 */
    clienterror(fd, filename, "404", "Not found",
            "Tiny couldn't find this file");
    return;
  }

  if (is_static) { /* Serve static content */
    /* 
      S_ISREG: regular file이 아니라면 false
      S_IRUSR: 읽기 권한이 없으면 false
    */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read the file");
      return;
    }
    /* 파일 실행 없이 단순 전송 */
    serve_static(buf, fd, filename, sbuf.st_size);
  }
  else { /* Serve dynamic content */
    /* S_IXUSR: 파일 실행 권한이 있는지 확인 */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program");
      return;
    }
    /* 동적 컨텐츠(CGI)를 실행해서 그 결과를 클라이언트에게 보낸다 */
    serve_dynamic(fd, filename, cgiargs, version); 
  }
}

void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE]; /* 8192 크기의 버퍼 생성 */
    
    /* rp에서 한 줄 \n 문자까지 읽어 buf에 저장 */
    Rio_readlineb(rp, buf, MAXLINE); 
    while(strcmp(buf, "\r\n")) { /* 버퍼가 \r\n 일때 */
      Rio_readlineb(rp, buf, MAXLINE); 
      printf("%s", buf);
    }
    return;   
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    /* strstr가 0인 경우 uri에서 "cgi-bin"을 찾지 못한 경우 */
    if (!strstr(uri, "cgi-bin")) { /* Static content */
        strcpy(cgiargs, "");
        strcpy(filename, "."); /* 현재 디렉토리 위치를 위한 '.' */
        strcat(filename, uri); /* ./filename 같은 구조 만들기*/
        if (uri[strlen(uri)-1] == '/')
            strcat(filename, "home.html");
        return -1;
    }
    else { /* Dynamic content */  
        ptr = index(uri, '?'); /* query parameter가 있는지 확인 */
        if (ptr) { /* 있는 경우 */
          /* 있는 경우 '?' 다음 오는 쿼리를 cgiargs에 담는다 */
          strcpy(cgiargs, ptr+1);
          
          *ptr = '\0'; /* uri에서 '?' 부분을 제거하여 순수한 경로만 남김 */
        } else { /* NULL인 경우 */
          strcpy(cgiargs, "");
        }
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}

void serve_static(int fd, char *filename, int filesize, char *version)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];  

  /* Send response headers to client */
  get_filetype(filename, filetype); /* filename에 따라 filetype을 받아온다 */
  sprintf(buf, "%s 200 OK\r\n", version); 
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype); /* \r\n\r\n 마지막 \r\n이 헤더 종료를 나타내는 빈 줄 */
  /* buf의 데이터를 buf 크기만큼 fd에 담을 것이다. */
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0); /* 읽기 전용, mode = 0 (의미 없음) */
  /* 
    filesize: 메모리 할당 크기
    PROT_READ: 읽기 가능
    MAP_PRIVATE: 사적 메모리 영역 매핑
    srcfd: 이 파일을 file descriptor로 지정
    offset: 0
  */ 
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); 
  /*
    11.8 mmap과 rio_readn 대신에, malloc, rio_readn, rio_writen을 사용한다.
  */
  srcp = malloc(filesize);
  Rio_readn(srcfd, srcp, filesize); /* srcfd 파일에서 srcp로 데이터 읽어오기 */
  
  Close(srcfd);
  Rio_writen(fd, srcp, filesize); /* scrp 데이터를 fd 클라이언트에게 전송 */
  
  // Munmap(srcp, filesize); /* unmap */
  free(srcp);
}

/*
  get_filetype - Derive fiel type from filename
*/
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, "mp4"))
    strcpy(filetype, "video/mp4");
  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs, char *version)
{
  char buf[MAXLINE], *emptylist[] = { NULL }; 

  /* Return first part of HTTP response */
  sprintf(buf, "%s 200 OK\r\n", version);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0) { /* Child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1); /* 환경변수 QUERY_STRING에 cgiargs 데이터를 넣는다. */
    Dup2(fd, STDOUT_FILENO); /* Redirect stdout to client*/
    Execve(filename, emptylist, environ); /* Run CGI Program */
  }
  Wait(NULL); /* Parent waits for and reaps child */
}

void response_requesthdrs(int fd, char *buf, char *version, char *filename, int filesize) {
  char filetype[MAXLINE];
  get_filetype(filename, filetype);
  
  sprintf(buf, "%s 200 OK\r\n", version); 
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype); /* \r\n\r\n 마지막 \r\n이 헤더 종료를 나타내는 빈 줄 */
    
  Rio_writen(fd, buf, strlen(buf));
  return;   
}