#include "csapp.h"

void echo(int connfd);

int main(int argc, char **argv)
{  
    /* Listen Socket Descriptor ,Connection Socket Descriptor */
    int listenfd, connfd;
    socklen_t clientlen; /* __U32_TYPE 정수 */
    struct sockaddr_storage clientaddr; /*  IPv4/IPv6를 구분하지 않고 처리할 수 있다. */
    char client_hostname[MAXLINE], client_port[MAXLINE];
    
    if (argc != 2) {
        /* 사용자가 포트 번호를 인자로 전달하지 않으면 에러 출력 후 종료 */
        fprintf(stderr, "usage: %s <port>\n", argv[0]); 
        exit(0);
    }

    
    listenfd = open_listenfd(argv[1]); /*  listen file descriptor */

    /* 반복하면서 클라이언트의 요청을 기다린다. */
    while (1) {
        /* 연결된 클라이언트의 주소 정보를 받아오기 때문에 주소 정보를 저장할 메모리 크기 지정 */
        clientlen = sizeof(struct sockaddr_storage);

        /* listenfd는 clientfd와 새로운 file descriptor connfd를 만든다. */
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        
        /* clientaddr에 저장된 소켓 주소 구조체를 문자열로 변환 */
        getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE,
                    client_port, MAXLINE, 0);

        printf("Connected to (%s, %s)\n", client_hostname, client_port);

        echo(connfd);
        close(connfd);
    }
    exit(0);
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