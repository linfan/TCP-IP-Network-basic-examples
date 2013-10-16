#include <sys/types.h>     // system types
//#include <sys/socket.h>    // socket API
//#include <netinet/in.h>    // struct sockaddr_in
//#include <arpa/inet.h>     // inet_addr()
#include <unistd.h>        // system call
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
extern int errno;
#define BUFFSIZE 128

int errexit(const char* format, ...);  // Print error information


// Handle socket session
void TCPdaytime(const char* host, const char* service)
{
    char buf[BUFFSIZE + 1];
    int s, n;                         // Client-end socket descriptor, receive lenth
    s = clientsock(host, service, "TCP", NULL);       // Create passive TCP socket
    fprintf(stderr, "[CLIENT] socket %d created.\n", s);
    sleep(1);
    while((n = recv(s, buf, BUFFSIZE, 0)) > 0)
    {
        fprintf(stderr, "[CLIENT] %d data read: \n", n);
        buf[n] = '\0';                // Add end of string symbol
        (void) fputs(buf, stderr);    // Show received data to stdout
    }
    fprintf(stderr, "[CLIENT] data receive finish. \n");
}

// Main function
int main(int argc,char* argv[])
{
    char *host = "127.0.0.1";         // was "localhost"
    char *service = "10086";          // was "daytime"
    switch (argc)
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
            errexit("usage: TCPdaytime [host [port]]\n");
    }
    TCPdaytime(host, service);
}

