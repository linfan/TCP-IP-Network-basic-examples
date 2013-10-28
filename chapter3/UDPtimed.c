// system types
#include <sys/types.h>  
// system socket define
#include <sys/socket.h> 
// struct sockaddr_in
#include <netinet/in.h> 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
extern int errno;

// Time difference between Unit time and Internet time 
#define UNIXEPOCH 2208988800UL

// Print error information
int errexit(const char* format, ...);
int passivesock(const char* service, const char* transport, int qlen);

// Create passive UDP socket
int passiveUDP(const char *service)
{
    return passivesock(service, "UDP", 0);
}

// Main function
int main(int argc, char *argv[])
{
    // Serice-end socket address
    struct sockaddr_in fsin;  
    // Service name (port number), was "time"
    char *service = "10010";  
    // Store client request, we don't care about the real request content,
    char buf[1];              
                               // and always reply the system time to client. only 1 byte buffer needed.
    // Service-end socket descriptor
    int sock;                 
    // Current time
    time_t now;               
    // Service-end socket struct length
    unsigned int alen;        
    switch (argc)
    {
        case 1:
            break;
        case 2:
            service = argv[1];
            break;
        default:
            errexit("usage: UDPtimed [port]\n");
    }
    sock = passiveUDP(service);
    while(1)
    {
        alen = sizeof(fsin);
        if (recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)&fsin, &alen) < 0)
            errexit("recvfrom: %s \n", strerror(errno));
        // Force convert time() to void type, thus it won't report warning
        (void) time(&now);    
        // Convert to internet time
        now = htonl((unsigned long)(now + UNIXEPOCH));
        (void) sendto(sock, (char*)&now, sizeof(now), 0, (struct sockaddr*)&fsin, sizeof(fsin));
    }
}

