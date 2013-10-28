// system types
#include <sys/types.h>    
// socket API
#include <sys/socket.h>   
// struct sockaddr_in
#include <netinet/in.h>   
// system call
#include <unistd.h>       
// inet_addr()
#include <arpa/inet.h>    
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
extern int errno;

// Create client TCP socket
unsigned int portbase = 10000;
int clientsock(const char* host, const char* service, const char* transport, struct sockaddr_in* fsin)
{
    struct servent *pse;
    struct protoent *ppe;
    struct sockaddr_in *sin;
    if (fsin == NULL)
        // Service address
        sin = malloc(sizeof(struct sockaddr_in));  
    else
        sin = fsin;
    int s, type;
    memset(sin, 0, sizeof(*sin));
    // assign sin_family
    sin->sin_family = AF_INET;                   
    // assign sin_addr
    sin->sin_addr.s_addr = inet_addr(host);      
    //echo("[CLIENT] Debug: service host : %lu (%s) \n", (unsigned long)sin->sin_addr.s_addr, host);
    // assign sin_port
    if (pse = getservbyname(service, transport))
        sin->sin_port = htons(ntohs((unsigned short)pse->s_port) + portbase);
    else if((sin->sin_port = htons((unsigned short)atoi(service))) == 0)
        errexit("can't get \"%s\" service entry\n", service);
    //echo("[CLIENT] Debug: service port : %u (%s) \n", sin->sin_port, service);
    // protocol
    if ((ppe = getprotobyname(transport)) == 0)
        errexit("can't get \"%s\" protocol entry\n", transport);
    if (strcmp(transport, "udp") == 0 || strcmp(transport, "UDP") == 0)
        type = SOCK_DGRAM;
    else
        type = SOCK_STREAM;
    s = socket(PF_INET, type, ppe->p_proto);
    if (s < 0)
        errexit("can't create socket: %s\n", strerror(errno));
    if (type == SOCK_STREAM)
    {
        //echo("[CLIENT] Debug: connecting to service %s.. \n", service);
        if (connect(s, (struct sockaddr*)sin, sizeof(*sin)) != 0)
            errexit("can't connect to service: %s\n", strerror(errno));
        //echo("[CLIENT] Debug: service %s connected. \n", service);
    }
    if (fsin == NULL)
        free(sin);
    return s;
}

