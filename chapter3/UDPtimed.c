#include <sys/types.h>   // system types
#include <sys/socket.h>  // system socket define
#include <netinet/in.h>  // struct sockaddr_in
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
extern int errno;

#define UNIXEPOCH 2208988800UL // Time difference between Unit time and Internet time 

int errexit(const char* format, ...); // Print error information
int passivesock(const char* service, const char* transport, int qlen);

// Create passive UDP socket
int passiveUDP(const char *service)
{
    return passivesock(service, "UDP", 0);
}

// Main function
int main(int argc, char *argv[])
{
    struct sockaddr_in fsin;   // Serice-end socket address
    char *service = "10010";   // Service name (port number), was "time"
    char buf[1];               // Store client request, we don't care about the real request content,
                               // and always reply the system time to client. only 1 byte buffer needed.
    int sock;                  // Service-end socket descriptor
    time_t now;                // Current time
    unsigned int alen;         // Service-end socket struct length
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
        (void) time(&now);     // Force convert time() to void type, thus it won't report warning
        now = htonl((unsigned long)(now + UNIXEPOCH)); // Convert to internet time
        (void) sendto(sock, (char*)&now, sizeof(now), 0, (struct sockaddr*)&fsin, sizeof(fsin));
    }
}

