#include <sys/types.h>   // system types
#include <sys/socket.h>  // system socket define
#include <netinet/in.h>  // struct sockaddr_in
#include <time.h>        // ctime(), without this headfile, we'll get a warning while compile and a segment fault when running
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>      // unix system call, e.g. write(), read()

#define UNIXEPOCH 2208988800UL // Time difference between Unit time and Internet time 
#define BUFSIZE 64
#define MSG "what time is it ?\n"
extern int errno;

int errexit(const char* format, ...); // Print error information

int connectUDP(const char *host, const char *service, struct sockaddr_in* sin)
{
    return clientsock(host, service, "UDP", sin);
}

// Main function
int main(int argc, char *argv[])
{
    char *host = "127.0.0.1";   // Service IP, was "localhost"
    char *service = "10010";    // Sercice name/port, was "time"
    time_t now;                 // Store replied time
    int s, n;                   // Socket descriptor and read data length
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

    struct sockaddr_in sin;     // Service-end socket address
    s = connectUDP(host, service, &sin);     // conncet to service

    fprintf(stderr, "[CLIENT] sending message to service..\n");
    // write to service by socket
    (void) sendto(s, MSG, strlen(MSG), 0, (struct sockaddr*)&sin, sizeof(sin));

    fprintf(stderr, "[CLIENT] message sent to service, waiting for respond..\n");
    // wait and read a package from socket
    int recvlen = sizeof(sin);
    n = recvfrom(s, (char*)&now, sizeof(now), 0, (struct sockaddr*)&sin, &recvlen);
    if (n < 0)
        errexit("read failed: %s\n", strerror(errno));

    now = ntohl((unsigned long)now);  // Convert to host byte order
    now -= UNIXEPOCH;                 // Convert to host time
    fprintf(stderr, "[CLIENT] now time is : %s\n", ctime(&now));

    return 0;
}

