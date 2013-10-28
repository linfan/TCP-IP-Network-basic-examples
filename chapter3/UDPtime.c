// system types
#include <sys/types.h>  
// system socket define
#include <sys/socket.h> 
// struct sockaddr_in
#include <netinet/in.h> 
// ctime(), without this headfile, we'll get a warning while compile and a segment fault when running
#include <time.h>       
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
// unix system call, e.g. write(), read()
#include <unistd.h>     

// Time difference between Unit time and Internet time 
#define UNIXEPOCH 2208988800UL
#define BUFSIZE 64
#define MSG "what time is it ?\n"
extern int errno;

// Print error information
int errexit(const char* format, ...);

int connectUDP(const char *host, const char *service, struct sockaddr_in* sin)
{
    return clientsock(host, service, "UDP", sin);
}

// Main function
int main(int argc, char *argv[])
{
    // Service IP, was "localhost"
    char *host = "127.0.0.1";  
    // Sercice name/port, was "time"
    char *service = "10010";   
    // Store replied time
    time_t now;                
    // Socket descriptor and read data length
    int s, n;                  
    switch(argc)
    {
        case 1:
            break;
        case 2:
            host = argv[1];
            break;
        case 3:
            host = argv[1];
            service = argv[2];
            break;
        default:
            errexit("usage: UDPtime [host [port]]\n");
    }

    // Service-end socket address
    struct sockaddr_in sin;    
    // conncet to service
    s = connectUDP(host, service, &sin);    

    fprintf(stderr, "[CLIENT] sending message to service..\n");
    // write to service by socket
    (void) sendto(s, MSG, strlen(MSG), 0, (struct sockaddr*)&sin, sizeof(sin));

    fprintf(stderr, "[CLIENT] message sent to service, waiting for respond..\n");
    // wait and read a package from socket
    int recvlen = sizeof(sin);
    n = recvfrom(s, (char*)&now, sizeof(now), 0, (struct sockaddr*)&sin, &recvlen);
    if (n < 0)
        errexit("read failed: %s\n", strerror(errno));

    // Convert to host byte order
    now = ntohl((unsigned long)now); 
    // Convert to host time
    now -= UNIXEPOCH;                
    fprintf(stderr, "[CLIENT] now time is : %s\n", ctime(&now));

    return 0;
}

