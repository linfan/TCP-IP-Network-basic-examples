#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>

#define MAXLINE   10
#define MAXBTYE   4096

int clientsock(const char* host, const char* service, const char* transport, struct sockaddr_in* fsin);
ssize_t readn(int fd, void* vptr, size_t n);

int connectTCP(const char* host, const char* port)
{
    return clientsock(host, port, "TCP", NULL);
}

int main(int argc,char* argv[])
{
    int i, j, fd;
    pid_t pid;
    ssize_t n;
    char request[MAXLINE], reply[MAXBTYE];
    char *hostname = "127.0.0.1";
    char *port = "10011";
    int nchildren = 10;
    int nloops = 500;
    int nbytes = 4000;
    if (argc != 6)
        echo("usage : client <hostname or IPaddr> <port> <#children> <#loops/child> <#bytes/request>\n");
    else
    {
        hostname = argv[1];
        port = argv[2];
        nchildren = atoi(argv[3]);
        nloops = atoi(argv[4]);
        nbytes = atoi(argv[5]);
    }
    snprintf(request, sizeof(request), "%d", nbytes);
    for (i = 0; i < nchildren; ++i)
    {
        if ((pid = fork()) == 0)
        {
            for (j = 0; j < nloops; ++j)
            {
                fd = connectTCP(hostname, port);
                //echo("[CLIENT] Debug: sending request %d bytes.\n", nbytes);
                send(fd, request, strlen(request), 0);
                //echo("[CLIENT] Debug: request sended.\n");
                if ((n = readn(fd, reply, nbytes)) != nbytes)
                    echo("[CLIENT] Error: request %d bytes, received %d bytes\n", nbytes, n);
                //echo("[CLIENT] Debug: received %d bytes reply.\n", n);
                close(fd);
            }
            echo("[CLIENT] child %d done.\n", i);
            exit(0);
        }
        while(wait(NULL) > 0)
            ;
        if (errno != ECHILD)
            errexit("[CLIENT] wait failed.\n");
    }
    return 0;
}

ssize_t readn(int fd, void* vptr, size_t n)
{
    size_t nleft;
    ssize_t nread;
    char* ptr;
    ptr = vptr;
    nleft = n;
    // read as much as possible each time, until reach n bytes
    while (nleft > 0)
    {
        if ((nread = recv(fd, ptr, nleft, 0)) < 0)
        {
            if (errno == EINTR)
                nread = 0;
            else
                return -1;
        }
        nleft -= nread;
        ptr += nread;
    }
    if (n - nleft < 0)
        echo("[CLIENT] Error: readn error.\n");
    return (n - nleft);
}

