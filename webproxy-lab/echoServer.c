#include "csapp.h"

void echo(int connfd);

/*
    argc: argument count — 프로그램 실행 시 전달된 인자의 개수
    argv: argument vector — 문자열 배열. 실행 인자들이 문자열로 저장됨.
    
    argv[0]: hostname이 들어올 것이고,
    argv[1]: port가 들어올 것이다.
*/
int main(int argc, char **argv)
{  
    /* Listen Socket Descriptor ,Connection Socket Descriptor */
    int listenfd, connfd;
    /* 클라이언트 길이 */
    socklen_t clientlen; 
    /* 
        왜 sockaddr_storage 타입이지?
        구조체를 쓰면 getaddrinfo, accept, recvfrom
        같은 함수들과 함께 IPv4/IPv6를 구분하지 않고
        동일한 방식으로 처리할 수 있어. 그래서 네트워크 
        프로그래밍에서 범용성을 위해 이 타입을 사용하는 것이 좋은 습관이야.
    */
    struct sockaddr_storage clientaddr;
    /* 
        클라이언트 호스트 이름, 클라이언트 포트 
        getnameinfo()에서 값을 받아올 것이다.
    */
    char client_hostname[MAXLINE], client_port[MAXLINE];
    
    if (argc != 2) {
        /* 
            사용자가 포트 번호를 인자로 전달하지 않으면 에러 출력 후 종료 
            예: usage: ./server 8080
        */
        fprintf(stderr, "usage: %s <port>\n", argv[0]); 
        exit(0);
    }

    /* 
        argv[1]는 port이다.
        open_listenfd(char *port)
        1. 지정된 포트로 소켓을 만들고,
        2. bind() (주소 할당)
        3. listen() 까지 해서
        4. 클라이언트 연결을 기다릴 수 있게 준비된 소켓 디스크립터를 반환한다.
    */
    listenfd = open_listenfd(argv[1]); 

    /* 반복하면서 클라이언트의 요청을 기다린다. */
    while (1) {
        /* 연결된 클라이언트의 주소 정보를 받아오기 때문에 주소 정보를 저장할 메모리 크기 지정 */
        clientlen = sizeof(struct sockaddr_storage);
        /*
            listenfd: 대기 중인 소켓
            accept()
            1. 연결 요청이 오면 해당 클라이언트와 데이터 송수신할 수 있는 새로운 소켓을 반환
            2. 클라이언트 주소는 clientaddr에 채워진다. 그래서 포인터 사용
        */
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        
        /*
            getnameinfo():
            - clientaddr에 저장된 IP주소, 포트를 문자열로 변환

            clientaddr: sockaddr* 으로 다운 캐스팅
            clientlen: 주소의 크기
            client_hostname: IP 주소 문자열이 저장될 버퍼
            MAXLINE: 버퍼 크기
            client_port: 포트 문자열이 저장될 버퍼
            0: 플래그 (예: NI_NUMERICHOST 주면 숫자 주소만 받음)
        */
        getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE,
                    client_port, MAXLINE, 0);
        /* 
            예시 출력
            Connected to (192.168.0.1, 51234)
        */
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        /*
            echo()
            - 클라이언트로부터 데이터를 읽고,
            - 받은 데이터를 그대로 돌려 보냄 (echo)
            - 연결이 종료될 때까지 반복
        */
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
    /*
        - 클라이언트로부터 한 줄씩 데이터를 읽어와서 buf에 저장
        - 읽어온 바이트 수를 n에 저장
        - n이 0이 아닌 동안(즉, 데이터가 있는 동안) 반복
    */
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", (int)n); /* 서버가 받은 바이트 수를 콘솔에 출력 */
        /* 
            - 받은 데이터를 그대로 클라이언트에게 다시 전송 (echo 기능) 
            - buf의 내용을 n바이트만큼 소켓에 쓰기
        */
        Rio_writen(connfd, buf, n); 
    }
}