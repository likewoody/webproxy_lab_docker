#include "csapp.h"

void echo(int connfd);

void sigchild_handler(int sig) 
{
    // waitpid
    // If PID is (pid_t) -1, match any process.
    // If PID is (pid_t) 0, match any process with the
    // same process group as the current process.
    // If PID is less than -1, match any process whose
    // process group is the absolute value of PID.
    // If the WNOHANG bit is set in OPTIONS, and that child
    // is not already dead, return (pid_t) 0. If successful,
    // return PID and store the dead child's status in STAT_LOC.
    // Return (pid_t) -1 for errors. If the WUNTRACED bit is
    // set in OPTIONS, return status for stopped children; otherwise don't.

    /*
        1. __pid_t __pid: -1 아무 프로세스나 다 받겠다.
        2. int *__stat_loc: 0(NULL) 자식의 종료 상태를 받아 올 필요가 없다.
        3. int __options: 
            - Wait No Hang 기다리지 않겠다.
            - 만약 종료된 자식 프로세스가 없다면 블로킹되지 않고 즉시 0을 반환합니다.
            - 종료된 자식이 있으면 해당 자식의 PID를 반환합니다.
    */
    // 좀비 프로세스 일괄 정리용
    while (waitpid(-1, 0, WNOHANG) > 0);
    

    return;
}

int main(int argc, char **argv)
{  
    /* Listen Socket Descriptor ,Connection Socket Descriptor */
    int listenfd, connfd;
    socklen_t clientlen = sizeof(struct sockaddr_in); /* __U32_TYPE 정수 */
    struct sockaddr_storage clientaddr; /*  IPv4/IPv6를 구분하지 않고 처리할 수 있다. */
    char client_hostname[MAXLINE], client_port[MAXLINE];
    
    if (argc != 2) {
        /* 사용자가 포트 번호를 인자로 전달하지 않으면 에러 출력 후 종료 */
        fprintf(stderr, "usage: %s <port>\n", argv[0]); 
        exit(0);
    }

    /*
        SIGCHLD: 자식 프로세스가 종료될 때 부모에게 전송되는 시그널
        sigchild_handler: 시그널을 받았을 때 처리할 함수
    */
    Signal(SIGCHLD, sigchild_handler);
    listenfd = open_listenfd(argv[1]); /*  listen file descriptor */

    /* 반복하면서 클라이언트의 요청을 기다린다. */
    while (1) {
        /* listenfd는 clientfd와 새로운 file descriptor connfd를 만든다. */
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        /* 자식 if문 함수 실행, 부모 if 조건에 맞지 않기 때문에 false 자식을 기다리지 않는다. */
        if (Fork() == 0) {
            Close(listenfd);    /* child closes its listening socket */
            echo(connfd);       /* child services client */
            close(connfd);      /* child closes connection with client */
            exit(0);
        }
        Close(connfd); /* Parent closes connected socket (important!)*/
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