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
#define BUF_MAX 1000
#define MAX_PARAM_NUM 3  
#define SA struct sockaddr
#define LISTENQ 1024

int getDataIndex(char *buf, int n)
{
    char *ptr = strtok(buf, " ");
    //printf("[DEBUG] n, tn: %d, %d\n", n, n-strlen(ptr)-1);
    n -= strlen(ptr); // the seq number
    n--; //the space
    //ptr = strtok(NULL, "");
    //strcpy(buf, ptr);
    return n;
}

void openFile(char *name, FILE **pfile)
{
    *pfile = fopen(name, "wb");
    if(NULL == *pfile)
    {
        printf("open file failed.\n");
        exit(2);
    }
}

int checkACK(char *buf, int i)
{
    char *ptr;
    char tmp[BUF_MAX+20];
    strcpy(tmp, buf);
    ptr = strtok(tmp, " ");
    //printf("recvi: i => %d : %d\n",strtol(ptr, NULL, 10), i);
    int recvi = strtol(ptr, NULL, 10);
    if(recvi < i )
        return 2;
    else if(recvi == i)
        return 1;
    else
        return 0;
}

void do_something(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen)
{
    int n;
    socklen_t len;
    char recvbuf[BUF_MAX+20], sendbuf[BUF_MAX+20];
    char fileName[50], fileLength[50];  //fileLenght enough?
    FILE *pfile;
    int file_size;
    char ack[5] = "ACK";
    char dataPath[10] = "./data/"; /* where the file saved*/
    len = clilen;
    n = recvfrom(sockfd, fileName, 50, 0, pcliaddr, &len);  // recv file name
    fileName[n] = '\0'; /* add null terninal, like raw data to c string */
    sendto(sockfd, ack, strlen(ack), 0, pcliaddr, len);
    len = clilen;
    n = recvfrom(sockfd, fileLength, 50, 0, pcliaddr, &len); //recv file length
    fileLength[n] = '\0';
    sendto(sockfd, ack, strlen(ack), 0, pcliaddr, len); 
    strcat(dataPath, fileName);
    openFile(dataPath, &pfile);
    printf("file name: %s\n", dataPath);
    printf("file length: %s\n", fileLength);
    fflush(stdout);
    file_size = strtol(fileLength, NULL, 10);
    printf("file length: %d\n", file_size);
    int i = 1;
    do {
        len = clilen;
        n = recvfrom(sockfd, recvbuf, BUF_MAX+20, 0, pcliaddr, &len);
        //printf("[DEBUG] raw data: %s\n", recvbuf);
        int ret = checkACK(recvbuf, i);
        if(0 == ret){
            printf("Some error ocurr\n");
            continue;
        }else if(1 == ret){
            sendto(sockfd, ack, strlen(ack), 0, pcliaddr, len);
            int dataN = getDataIndex(recvbuf, n);
            fwrite(recvbuf+(n-dataN), sizeof(char), dataN, pfile);
            file_size -= dataN;
            printf("%d: ACK(remaining: %d)\n", i, file_size);
            i++;
        }else if(2 == ret){
            sendto(sockfd, ack, strlen(ack), 0, pcliaddr, len);
            printf("ACK lost, re-send ack to sender\n");
        }
    } while(file_size);
    fclose(pfile);
}

int main(int argc, char **argv)
{
    int sockfd;
    int addrlen = sizeof(struct sockaddr_in);
    struct sockaddr_in servaddr, cliaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    int size = 100000; //max 212992, kernel will double the size !!
    //int rcvBufferSize;
    //int sockOptSize = sizeof(rcvBufferSize);
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    //getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvBufferSize, &sockOptSize);
    //printf("socket receive buf %d\n", rcvBufferSize);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(8889);

    bind(sockfd, (struct sockaddr *) &servaddr, addrlen);
    do_something(sockfd, (struct sockaddr *) &cliaddr, addrlen);

}
