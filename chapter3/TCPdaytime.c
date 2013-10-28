// system types
#include <sys/types.h>    
// socket API
//#include <sys/socket.h>   
// struct sockaddr_in
//#include <netinet/in.h>   
// inet_addr()
//#include <arpa/inet.h>    
// system call
#include <unistd.h>       
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
extern int errno;
#define BUFFSIZE 128

// Print error information
int errexit(const char* format, ...); 


// Handle socket session
void TCPdaytime(const char* host, const char* service)
{
    char buf[BUFFSIZE + 1];
    // Client-end socket descriptor, receive lenth
    int s, n;                        
    // Create passive TCP socket
    s = clientsock(host, service, "TCP", NULL);      
    fprintf(stderr, "[CLIENT] socket %d created.\n", s);
    sleep(1);
    while((n = recv(s, buf, BUFFSIZE, 0)) > 0)
    {
        fprintf(stderr, "[CLIENT] %d data read: \n", n);
        // Add end of string symbol
        buf[n] = '\0';               
        // Show received data to stdout
        (void) fputs(buf, stderr);   
    }
    fprintf(stderr, "[CLIENT] data receive finish. \n");
}

// Main function
int main(int argc,char* argv[])
{
    // was "localhost"
    char *host = "127.0.0.1";        
    // was "daytime"
    char *service = "10086";         
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

