/*
    12.2.1 I/O 다중화에 기초한 동시성 이벤트 기반 서버
*/

#include "csapp.h"

void init_pool(int listenfd, pool *p);
void add_client(int connfd, pool *p);

typedef struct {        /* Represents a pool of connected descriptors */
    int maxfd;                  /* Largest descriptor in read_set */
    fd_set read_set;            /* Set of all active descriptors */
    fd_set ready_set;           /* Subset of desciprtors ready for reading */
    int nready;                 /* Number of ready descriptors from select */
    int maxi;                   /* High water(최고점 or 기준점) index into client array */
    int clientfd[FD_SETSIZE]; // 1024   /* Set of active descriptors */
    rio_t clientrio[FD_SETSIZE]         /* Set of active read buffers */
} pool;

int byte_cnt = 0; /* Counts total bytes received by server */

int main(int argc, char **argv)
{  
    /* Listen Socket Descriptor ,Connection Socket Descriptor */
    int listenfd, connfd;
    socklen_t clientlen; /* __U32_TYPE 정수 */
    struct sockaddr_storage clientaddr; /*  IPv4/IPv6를 구분하지 않고 처리할 수 있다. */
    pool pool;
    
    if (argc != 2) {
        /* 사용자가 포트 번호를 인자로 전달하지 않으면 에러 출력 후 종료 */
        fprintf(stderr, "usage: %s <port>\n", argv[0]); 
        exit(0);
    }

    listenfd = open_listenfd(argv[1]);  /*  listen file descriptor */
    init_pool(listenfd, &pool); /* fd와 pool 연결 (pool 초기화) */

    while (1) {
        /* Wait for listening/connected descriptor(s) to become ready */
        pool.ready_set = pool.read_set;
        /* 파일 디스크럽트의 갯수 반환 */
        pool.nready = Select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);

        /* If listenfd is ready, add new client to pool */
        if (FD_ISSET(listenfd, &pool.ready_set)) {
            clientlen = sizeof(struct sockaddr_storage);
            /* lstenfd를 client와 연결하여 connfd로 반환*/
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            add_client(connfd, &pool);
        }
        /* Echo a text line from each ready connected descriptor */
        check_clients(&pool);
    }
}

void init_pool(int listenfd, pool *p) /* 클라이언트 풀 초기화 */
{
    /* Initially, there are not connected descriptors */
    p->maxi = -1;
    
    for (int i=0;i<FD_SETSIZE;i++) /* -1: 가용 공간 */
        p->clientfd[i] = -1;
    
    /* Initially, listenfd is only member of select read set */
    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);          /* Clear p->read_set to zero */
    FD_SET(listenfd, &p->read_set); /* Set listenfd to p->read_set */
}

/* 새로운 클라이언트를 살아있는 클라이언트 풀에 추가해준다. */
void add_client(int connfd, pool *p)
{
    p->nready--; /* 빈 공간 하나를 줄인다. 왜냐 사용할 것이기 때문 */
    for (int i=0; i<FD_SETSIZE; i++) /* Find an available slot */
    {
        if (p->clientfd[i] < 0) { /* 빈 공간 찾기 */
            /* Add connected descriptors to the pool */
            p->clientfd[i] = connfd;
            Rio_readinitb(&p->clientrio[i], connfd); /* p->clientrio[i]에 connfd 연결 초기화*/

            /* Add connfd to p->read_set */
            FD_SET(connfd, &p->read_set);

            /* Update max descriptor and pool highwater mark(최고점 or 기준점) */
            if (connfd > p->maxfd)
                p->maxfd = connfd;
            if (i > p->maxi)
                p->maxi = i;
            break;
        }

        if (i == FD_SETSIZE) /* Couldn't find and emtpy slot */
            app_error("add_client error: Too many clients");
    }
}

void check_clients(pool *p)
{
    int connfd, n;
    char buf[MAXLINE];
    rio_t rio;

    for (int i=0; (i<p->maxi) && (p->nready > 0); i++) {
        connfd = p->clientfd[i];
        rio = p->clientrio[i];

        /* If the descriptor is ready, echo a text line from it */
        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
            p->nready--;
            /* 
                rio를 통해 네트워크 스택에 저장되어 있는 내부 버퍼에 있는 데이터를 \n 혹은 
                EOF까지 읽어 buf에 저장한다.
            */
            if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
                byte_cnt += n;
                printf("Server received %d (%d total) bytes on fd %d\n", n, byte_cnt, connfd);
                Rio_writen(connfd, buf, n);
            }

            /* EOF detected, remove descriptor from pool */
            else {
                Close(connfd);
                FD_CLR(connfd, &p->read_set); /* read_set에 connfd가 있으면 비트를 0으로 설정 */
                p->clientfd[i] = -1;
            }
        }
    }
}