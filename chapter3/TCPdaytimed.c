#include <sys/types.h>     // system types
#include <sys/socket.h>    // socket API
#include <netinet/in.h>    // struct sockaddr_in
#include <unistd.h>        // system call
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
extern int errno;

int errexit(const char* format, ...);  // Print error information
int passivesock(const char* service, const char* transport, int qlen);

int passiveTCP(const char* service)    // Create passive TCP socket
{
    return passivesock(service, "TCP", 1);
}

// Handle temporary socket session
void TCPdaytimed(int fd)
{
    char *pts;                         // Store time string
    time_t now;                        // Current time
    (void) time(&now);
    pts = ctime(&now);
    (void) send(fd, pts, strlen(pts), 0);
}

// Main function
int main(int argc,char* argv[])
{
    struct sockaddr_in fsin;           // Service-end socket
    char *service = "10086";           // was "daytime"
    int msock, ssock;                  // permanent socket descriptor and temporary socket descriptor
    unsigned int alen;                 // socket length
    switch (argc)
    {
        case 1:
            break;
        case 2:
            service = argv[1];
            break;
        default:
            errexit("[SERVICE] usage: TCPdaytimed [port]\n");
    }
    fprintf(stderr, "[SERVICE] creating passive socket..\n");
    msock = passiveTCP(service);       // Create passive TCP socket
    fprintf(stderr, "[SERVICE] passive socket %d created.\n", msock);
    while(1)
    {
        alen = sizeof(fsin);
        // connect to first client, create temporary socket 
        fprintf(stderr, "[SERVICE] waiting for client..\n");
        ssock = accept(msock, (struct sockaddr*)&fsin, &alen);
        if (ssock < 0)
            errexit("accept failed: %s\n", strerror(errno));
        fprintf(stderr, "[SERVICE] temporary socket %d created.\n", ssock);
        // Use temporary socket interact with client
        TCPdaytimed(ssock);
        (void) close(ssock);
    }
    return 0;
}

