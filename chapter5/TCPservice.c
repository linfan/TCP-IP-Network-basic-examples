#include <sys/types.h>     // system types
#include <sys/socket.h>    // socket API
#include <netinet/in.h>    // struct sockaddr_in
#include <arpa/inet.h>     // inet_addr()
#include <unistd.h>        // system call
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
extern int errno;

#define PORT 10088
#define QLEN 10
#define BUFSIZE 1024

int errexit(const char* format, ...);  // Print error information
int echo(const char* format, ...);     // Print work information

// SIGCHLD handler
void sig_child(int sig)
{
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        echo("[SERVICE] Child %d terminated\n", pid);
    return;
}

// Handle request from client
void processClient(int connectfd, struct sockaddr_in client)
{
    int num, i;
    char recvbuf[BUFSIZE], sendbuf[BUFSIZE], clientName[BUFSIZE];
    int len;
    echo("[SERVICE] Accept a connect from %s.\n", inet_ntoa(client.sin_addr));
    num = recv(connectfd, clientName, BUFSIZE, 0);
    if (num == 0)              // Receive nothing, close socket
    {
        close(connectfd);
        echo("[SERVICE] Client disconnected.\n");
        return;
    }
    clientName[num - 1] = '\0';    // [num -1] to cut of the last '\n' received from client
    echo("[SERVICE] Client name is: %s.\n", clientName);
    while (num = recv(connectfd, recvbuf, BUFSIZE, 0))
    {
        recvbuf[num] = '\0';                     // receive every thing, end with '\n'
        echo("[SERVICE] Received client (%s) message: %s", clientName, recvbuf);
        for (i = 0; i < num - 1; i++)            // Revert the char order
        {
            sendbuf[i] = recvbuf[num - i - 2];   // subtract 2 to leave along the '\0' and '\n'
        }
        sendbuf[num - 1] = '\0';                 // The real data have only [num - 1] length
        len = strlen(sendbuf);
        send(connectfd, sendbuf, len, 0);        // Send respond to client
    }
    close(connectfd);
}

int passivesock(struct sockaddr_in* server)
{
    int listenfd;
    if ((listenfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        errexit("can't create socket: %s \n", strerror(errno));

    int opt = SO_REUSEADDR;    // Allow socket reuse local addr and port
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Initialize socket addr
    bzero(server, sizeof(*server));
    server->sin_family = AF_INET;
    server->sin_port = htons((unsigned short)PORT);
    server->sin_addr.s_addr = htonl(INADDR_ANY);   // here htonl() is optional

    // Bind socket to service-end address
    if (bind(listenfd, (struct sockaddr*)server, sizeof(*server)) < 0)
        errexit("can't bind to %s port: %s \n", PORT, strerror(errno));
    // For TCP socket, convert it to passive mode
    if (listen(listenfd, QLEN) < 0)
        errexit("can't listen on %s port: %s \n", PORT, strerror(errno));

    return listenfd;
}

// Main function
int main(int argc,char* argv[])
{
    int listenfd, connectfd;           // Main socket and servant socket
    pid_t pid;
    struct sockaddr_in server;         // Service-end socket addr
    struct sockaddr_in client;         // Client-end socket addr
    socklen_t ssize = sizeof(struct sockaddr_in);

    echo("[SERVICE] creating passive socket..\n");
    listenfd = passivesock(&server);
    echo("[SERVICE] passive socket %d created.\n", listenfd);

    signal(SIGCHLD, sig_child);        // Install signal hander

    while(1)
    {
        // connect to first client, create temporary socket 
        echo("[SERVICE] waiting for client..\n");
        if ((connectfd = accept(listenfd, (struct sockaddr*)&client, &ssize)) < 0)
            errexit("accept failed: %s\n", strerror(errno));
        echo("[SERVICE] servant socket %d created.\n", connectfd);
        pid = fork();
        if (pid > 0) // Parent process
        {
            (void) close(connectfd);
            continue;
        }
        else if (pid == 0) // Child process
        {
            (void) close(listenfd);
            // Use servant socket interact with client
            processClient(connectfd, client);
            exit(0);
        }
        else
        {
            errexit("fork failed: %s\n", strerror(errno));
        }
    }
    close(listenfd);
    return 0;
}

