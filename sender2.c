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
#define BUF_MAX 8192
#define MAX_PARAM_NUM 3  
#define SA struct sockaddr
#define LISTENQ 1024

int readable_timeo(int fd, int sec)
{
    fd_set rset;
    struct timeval tv;
    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    tv.tv_sec = sec;
    tv.tv_usec = 0;
    return (select(fd+1, &rset, NULL, NULL, &tv));
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
void sendFileName(int sockfd, struct sockaddr *pservaddr, int servlen, char *recvbuf, char *name)
{
    int n;
    while(1)
    {
        sendto(sockfd, name, strlen(name), 0, pservaddr, servlen);/* send file name */ 
        if(readable_timeo(sockfd, 1)==0)
        {
            //TIMEOUT
            printf("try to re-send: FILE NAME\n");
            fflush(stdout);
            continue;
        }else
        {
            n = recvfrom(sockfd, recvbuf, BUF_MAX, 0, NULL, NULL);
            printf("Name exchange done.\n");
            fflush(stdout);
            return;
        }
    }   
}
void sendFileLength(int sockfd, struct sockaddr *pservaddr, int servlen, char *recvbuf, char *sendbuf)
{
    //n = recvfrom(sockfd, recvbuf, BUF_MAX, 0, NULL, NULL); /* recv ACK */
    int n;
    while(1)
    {
        sendto(sockfd, sendbuf, strlen(sendbuf), 0, pservaddr, servlen);/* send file length */ 
        if(readable_timeo(sockfd, 1)==0)
        {
            printf("try to re-send: FILE SIZE\n");
            fflush(stdout);
            continue;
        }else
        {
            n = recvfrom(sockfd, recvbuf, BUF_MAX, 0, NULL, NULL);
            printf("Size exchange done.\n");
            fflush(stdout);
            return;
        }
    }   
}
void do_something(FILE *fp, char *name,  int sockfd, struct sockaddr *pservaddr, socklen_t servlen)
{
    int n, nread;
    char sendbuf[BUF_MAX+20], recvbuf[BUF_MAX+20]; // 20 is the space for header
    FILE *pfile;
    int file_size;
    connect(sockfd, (struct sockaddr *) pservaddr, servlen);
   
    /* META data exchange */
    //sendto(sockfd, name, strlen(name), 0, pservaddr, servlen);/* send file name */ 
    //n = recvfrom(sockfd, recvbuf, BUF_MAX, 0, NULL, NULL); /* recv ACK */
    openFile(name, &pfile);
    sendFileName(sockfd, pservaddr, servlen, recvbuf, name);
    file_size = filelength(&pfile);
    printf("%s\n", name);
    printf("%d\n", file_size);
    snprintf(sendbuf, BUF_MAX, "%d", file_size);
    //sendto(sockfd, sendbuf, strlen(sendbuf), 0, pservaddr, servlen); /* send file length*/
    //n = recvfrom(sockfd, recvbuf, BUF_MAX, 0, NULL, NULL); /* recv ACK */
    sendFileName(sockfd, pservaddr, servlen, recvbuf, sendbuf);
    /* META data exchange */

    int i =1;
    char tmpbuf[BUF_MAX];
    while(n = fread(tmpbuf, sizeof(char), BUF_MAX, pfile))
    {
        snprintf(sendbuf, 20, "%d ", i);
        int headerLength = strlen(sendbuf);
        memcpy(sendbuf+headerLength, tmpbuf, n); /* Use memcpy instead of strcpy, because data may not be string*/
        sendto(sockfd, sendbuf, n+headerLength, 0, pservaddr, servlen); /* send CONTENT of the file*/
        file_size -= n;
        nread = n;
        if(readable_timeo(sockfd, 1)==0)
        {
            printf("try to re-send: %d\n", i);
            fflush(stdout);
            fseek(pfile, -nread, SEEK_CUR);
            file_size += nread;
        }else
        {
            n = recvfrom(sockfd, recvbuf, BUF_MAX, 0, NULL, NULL);
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
