#include <sys/types.h>   // system types
#include <sys/socket.h>  // system socket define
#include <netinet/in.h>  // struct sockaddr_in
#include <netdb.h>       // Network related functions, e.g. gethostbyname()
#include <stdarg.h>      // va_list
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
extern int errno;

unsigned int portbase = 1024;     // Allow test multi-service on the machine using different port
int passivesock(const char* service, const char* transport, int qlen)
{
    struct servent *pse;       // Store service entry return from getservbyname()
    struct protoent *ppe;      // Store protocol entry return from getprotobyname()
    struct sockaddr_in sin;    // Service-end socket
    int s, type;               // Service-end socket descriptor and service type
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;  // TCP/IP suite
    sin.sin_addr.s_addr = INADDR_ANY;  // Use any local IP, need translate to internet byte order
    // Get port number
    if (pse = getservbyname(service, transport)) // service is service name
        sin.sin_port = htons(ntohs((unsigned short)pse->s_port) + portbase);
    else if((sin.sin_port = htons((unsigned short)atoi(service))) == 0) // service is port number
        errexit("can't get \"%s\" service entry\n", service);
    // Get protocol number
    if ((ppe = getprotobyname(transport)) == 0)
        errexit("can't get \"%s\" protocol entry\n", transport);
    // Tranport type
    if (strcmp(transport, "udp") == 0 || strcmp(transport, "UDP") == 0)
        type = SOCK_DGRAM;
    else
        type = SOCK_STREAM;
    fprintf(stderr, "[SERVICE] transport: %s, protocol: %d, port: %u, type: %d\n", \
            transport, ppe->p_proto, sin.sin_port, type);
    // Create socket
    s = socket(PF_INET, type, ppe->p_proto);
    if (s < 0)
        errexit("can't create socket: %s \n", strerror(errno));
    // Bind socket to service-end address
    if (bind(s, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        errexit("can't bind to %s port: %s \n", service, strerror(errno));
    // For TCP socket, convert it to passive mode
    if (type == SOCK_STREAM && listen(s, qlen) < 0)
        errexit("can't listen on %s port: %s \n", service, strerror(errno));
    return s;
}

