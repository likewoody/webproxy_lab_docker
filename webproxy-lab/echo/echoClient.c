#include "csapp.h"

int main(int argc, char **argv) {
    int clientfd; /* client socket descriptor */

    char *host, *port, buf[MAXLINE]; 
    rio_t rio;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    clientfd = open_clientfd(host, port); /* host와 port를 통해 clientfd 생성 */
    rio_readinitb(&rio, clientfd);

    /* EOF (Ctrl+D 또는 입력 스트림 종료)까지 반복. */
    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        rio_writen(clientfd, buf, strlen(buf)); /* listenfd와 연결을 시도하고 연결이 된다면 buf를 서버에 보낸다. */
        rio_readlineb(&rio, buf, MAXLINE); /* 서버로부터 받은 buf를 읽는다. */
        Fputs(buf, stdout); /* 서버로부터 받은 응답을 화면에 출력. */
    }
    close(clientfd);
    exit(0);
}