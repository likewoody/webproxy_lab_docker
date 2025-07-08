#include "csapp.h"

void echo(int connfd);
void command(void);

int main(int argc, char **argv)
{  
    /* Listen Socket Descriptor ,Connection Socket Descriptor */
    int listenfd, connfd;
    socklen_t clientlen; /* __U32_TYPE 정수 */
    struct sockaddr_storage clientaddr; /*  IPv4/IPv6를 구분하지 않고 처리할 수 있다. */
    char client_hostname[MAXLINE], client_port[MAXLINE];
    fd_set read_set, ready_set;
    
    if (argc != 2) {
        /* 사용자가 포트 번호를 인자로 전달하지 않으면 에러 출력 후 종료 */
        fprintf(stderr, "usage: %s <port>\n", argv[0]); 
        exit(0);
    }

    listenfd = open_listenfd(argv[1]);  /*  listen file descriptor */
    FD_ZERO(&read_set);                 /* Clear read set */
    // 키보드 입력을 모니터링하겠다는 의미
    FD_SET(STDIN_FILENO, &read_set);    /* Add stdin to read set */
    // 클라이언트 연결 요청을 모니터링하겠다는 의미
    FD_SET(listenfd, &read_set);        /* Add listenfd to read set*/

    /* 반복하면서 클라이언트의 요청을 기다린다. */
    while (1) {
        ready_set = read_set;
        /* 
            1. 모니터링할 파일 디스크립터의 최대값 +1
            SET한 값 중 큰 값인 listenfd를 사용
            2. fd를 담을 준비된 set
            5. 마지막 인자 NULL: 타임아웃 없음 = 무한 대기

            Select 동작
            - 블로킹 함수: 모니터링 중인 fd중 하나라도 준비될 때까지 대기
            - 준비되면 ready_set에 해당 fd만 남기고 나머지 제거
        */
        Select(listenfd+1, &ready_set, NULL, NULL, NULL);
        if (FD_ISSET(STDIN_FILENO, &ready_set))
            command(); /* Read command line from stdin */
        if (FD_ISSET(listenfd, &ready_set)) {
            clientlen = sizeof(struct sockaddr_storage);
            connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
            echo(connfd);
            Close(connfd); /* Parent closes connected socket (important!)*/
        }
    }
}

void echo(int connfd)
{
    size_t n; /* 읽어온 바이트 수를 저장할 변수 */
    char buf[MAXLINE]; /* 클라이언트로부터 받은 데이터를 저장할 버퍼 */
    rio_t rio; /* Robust I/O 구조체 (버퍼링된 읽기를 위한 구조체)*/

    Rio_readinitb(&rio, connfd); /* rio 구조체를 초기화하여 연결된 소켓과 연결 */
    
    /* 여기에서 BLOCKING이 발생하여 문자들어올 때까지 대기한다. */
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", (int)n); 
        Rio_writen(connfd, buf, n); 
    }
}

void command(void) 
{
    char buf[MAXLINE];
    
    if (!Fgets(buf, MAXLINE, stdin))
        exit(0); // EOF
    printf("%s", buf); /* Process the input command */
}