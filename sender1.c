#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include        <time.h>                /* old system? */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <netdb.h>
#include        <signal.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <sys/stat.h>    /* for S_xxx file mode constants */
#include        <sys/uio.h>             /* for iovec{} and readv/writev */
#include        <unistd.h>
#include        <sys/wait.h>
#include        <sys/un.h>              /* for Unix domain sockets */
#include        <errno.h>
#define BUF_MAX 1000
#define MAX_PARAM_NUM 3  
#define SA struct sockaddr
#define LISTENQ 1024

static void sig_alrm(int signo)
{
    printf("TIMEOUT\n");
    return;
}

void openFile(char *name, FILE **pfile)
{
	*pfile = fopen(name, "rb");
	if(NULL == *pfile)
	{
    	printf("open file failed.\n");
    	exit(2);
    }
}

int filelength(FILE **pfile)
{
    fseek(*pfile, 0, SEEK_END);
    int file_size = ftell(*pfile);
    fseek(*pfile, 0, SEEK_SET);
    return file_size;
}
void do_something(FILE *fp, char *name,  int sockfd, struct sockaddr *pservaddr, socklen_t servlen)
{
    int n, nread;
    char sendbuf[BUF_MAX+20], recvbuf[BUF_MAX+20]; // 20 is the space for header
    FILE *pfile;
    int file_size;
    connect(sockfd, (struct sockaddr *) pservaddr, servlen);
    
    sendto(sockfd, name, strlen(name), 0, pservaddr, servlen); 
    n = recvfrom(sockfd, recvbuf, BUF_MAX, 0, NULL, NULL);
    recvbuf[n] = 0;
    fputs(recvbuf, stdout);
    openFile(name, &pfile);
    file_size = filelength(&pfile);
    printf("%s\n", name);
    printf("%d\n", file_size);
    snprintf(sendbuf, BUF_MAX, "%d", file_size);
    sendto(sockfd, sendbuf, strlen(sendbuf), 0, pservaddr, servlen); 
    n = recvfrom(sockfd, recvbuf, BUF_MAX, 0, NULL, NULL);
    recvbuf[n] = 0;
    fputs(recvbuf, stdout);
    int i =1;
    char tmpbuf[BUF_MAX];
    /* timeout */
    signal(SIGALRM, sig_alrm);
    siginterrupt(SIGALRM, 1);
    while(n = fread(tmpbuf, sizeof(char), BUF_MAX, pfile))
    {
        tmpbuf[n] = '\0';
        printf("[DEBUG] RAW data: %s\n", tmpbuf);
        snprintf(sendbuf, 20, "%d ", i);
        int headerLength = strlen(sendbuf);
        //strncat(sendbuf, tmpbuf, BUF_MAX); //payload:i DATA 
        memcpy(sendbuf+headerLength, tmpbuf, n);
        printf("[DEBUG] CONCAT data: %s\n", sendbuf);
        //sendto(sockfd, tmpbuf, n, 0, pservaddr, servlen);
        sendto(sockfd, sendbuf, n+headerLength, 0, pservaddr, servlen);
        file_size -= n;
        nread = n;
        alarm(2);
        if((n = recvfrom(sockfd, recvbuf, BUF_MAX, 0, NULL, NULL)) < 0){
            if(errno == EINTR){
                printf("try to re-send: %d\n", i);
                fflush(stdout);
                fseek(pfile, -nread, SEEK_CUR);
                file_size += nread;
            }
            else{
                printf("recvfrom error\n");
                exit(2);
            }
        }else{
            alarm(0);
            recvbuf[n] = '\0';
            printf("%d: %s(remaining: %d)\n", i++, recvbuf, file_size);         
        }      
    }
}

int main(int argc, char **argv)
{
    int sockfd;
    int addrlen = sizeof(struct sockaddr_in);
    struct sockaddr_in servaddr;
    
    if(argc != 4)
    {
        printf("Usage: %s <SERVER_ADDR> <SERVER_PORT> <FILE_NAME>\n", argv[0]);
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
    servaddr.sin_port = htons(atoi(argv[2]));

    do_something(stdin, argv[3], sockfd, (struct sockaddr *) &servaddr, addrlen);

}
