#include "csapp.h"

/*
    왜 그냥 write()가 아니라 Rio_writen()일까?
    - 일반적인 write(fd, buf, n) 호출은 네트워크 상황에 따라 요청한 바이트보다 적게 보낼 수도 있어.
    - 예를 들어, write()가 100바이트 보내려 했는데 50바이트만 보내고 리턴할 수도 있어.
    - 그래서 Rio_writen()은 내부적으로 반복해서 write()를 호출해서 전부 보낼 때까지 끝까지 처리해줘.
*/

/*
    argc: argument count - 전달된 인자의 개수
    argv: argument vector

    argv[0]: 프로그램명
    argv[1]: host
    argv[2]: port
*/
int main(int argc, char **argv) {
    int clientfd; /* client socket descriptor */
    /*
        *host: server host
        *port: server port
        buf[MAXLINE]: 전송할 데이터 버퍼
        rio: Robust I/O 구조체 (버퍼링된 읽기를 위한 구조체)
    */
    char *host, *port, buf[MAXLINE]; 
    rio_t rio;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    /*
        open_clientfd(char *hostname, char *port)
        1. getaddinfo(): 문자열로 된 호스트명과 포트를 sockaddr 계열의 구조체로 변환하는 역할
        - 내부적으로 DNS 조회도 해주고, IPv4/IPv6 모두를 다룰 수 있게 설계되어 있다.
        2. socket(): 소켓 식별자 생성
        3. connect(): 클라이언트는 서버와 연결시도

        성공하면 socket file descriptor,
        실패하면 음수
    */
    clientfd = open_clientfd(host, port);
    rio_readinitb(&rio, clientfd);

    /*
        표준 입력(키보드)에서 한 줄씩 읽어서 buf에 저장
        EOF (Ctrl+D 또는 입력 스트림 종료)까지 반복.
    */
    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        /* 
            Rio_writen()
            - buf에 있는 내용을 소켓으로 전송
                - clientfd 소켓 파일 디스크립터를 통해 데이터를 전송한다
            - Rio_writen()은 요청한 바이트 수만큼
              다 보낼 때까지 내부적으로 write()를 반복 호출함
        */
        rio_writen(clientfd, buf, strlen(buf));
        /*
            Rio_readlineb()
            - 서버로부터 한 줄 읽어와 buf에 저장
            - Rio_readlineb()는 개행 문자가 나올 
              때까지 한 줄을 읽는 함수로, 내부적으로 버퍼링됨.
        */
        rio_readlineb(&rio, buf, MAXLINE);
        /*
            받은 데이터를 표준 출력 (터미널)에 출력.
            즉, 클라이언트에서 입력한 값을 서버로 보내고,
             서버로부터 받은 응답을 화면에 출력.
        */
        Fputs(buf, stdout);
    }
    close(clientfd);
    exit(0);
}