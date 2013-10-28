// system types
#include <sys/types.h>    
// socket API
#include <sys/socket.h>   
// struct sockaddr_in
#include <netinet/in.h>   
// system call
#include <unistd.h>       
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
extern int errno;

// Print error information
int errexit(const char* format, ...); 
int passivesock(const char* service, const char* transport, int qlen);

// Create passive TCP socket
int passiveTCP(const char* service)   
{
    return passivesock(service, "TCP", 1);
}

// Handle temporary socket session
void TCPdaytimed(int fd)
{
    // Store time string
    char *pts;                        
    // Current time
    time_t now;                       
    (void) time(&now);
    pts = ctime(&now);
    (void) send(fd, pts, strlen(pts), 0);
}

// Main function
int main(int argc,char* argv[])
{
    // Service-end socket
    struct sockaddr_in fsin;          
    // was "daytime"
    char *service = "10086";          
    // permanent socket descriptor and temporary socket descriptor
    int msock, ssock;                 
    // socket length
    unsigned int alen;                
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
    // Create passive TCP socket
    msock = passiveTCP(service);      
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

