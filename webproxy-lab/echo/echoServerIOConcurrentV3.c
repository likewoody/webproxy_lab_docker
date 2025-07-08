/*
    12.3.8 스레드에 기초한 동시성 서버
*/

#include "csapp.h"

void echo(int connfd);
void *thread(void *vargp);

int main(int argc, char **argv)
{  
    /* Listen Socket Descriptor ,Connection Socket Descriptor */
    int listenfd, *connfdp;
    socklen_t clientlen; /* __U32_TYPE 정수 */
    struct sockaddr_storage clientaddr; /*  IPv4/IPv6를 구분하지 않고 처리할 수 있다. */
    pthread_t tid;
    
    if (argc != 2) {
        /* 사용자가 포트 번호를 인자로 전달하지 않으면 에러 출력 후 종료 */
        fprintf(stderr, "usage: %s <port>\n", argv[0]); 
        exit(0);
    }

    listenfd = open_listenfd(argv[1]);  /*  listen file descriptor */

    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfdp = malloc(sizeof(int));
        /* lstenfd를 client와 연결하여 connfd로 반환*/
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Pthread_create(tid, NULL, thread, connfdp);
    }
}

/* Thread routine */
void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    echo(connfd);
    Close(connfd);
    return NULL;
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