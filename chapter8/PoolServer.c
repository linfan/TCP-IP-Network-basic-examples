#include <unistd.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>       // struct sockaddr_in
#include <pthread.h>

#define PORT      10011
#define LISTENQ   1024
#define MAXLINE   10         // Max received message bytes
#define MAXBTYE   4096       // Max reply message bytes
#define MAXCLIENT 32         // Max accepted client at the same time

typedef struct 
{
    pthread_t thread_tid;    // thread id
    long thread_count;       // connected client count
} Thread;

Thread *tptr;                // thread array (thread pool)
int clifd[MAXCLIENT], iget, iput;    // client socket descriptor array
int listenfd;                // socket descriptor in main thread
int nthreads;                // total thread count
pthread_mutex_t clifd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clifd_cond = PTHREAD_COND_INITIALIZER;

void pr_cpu_time(void);      // print user and system cpu time
void web_child(int);         // handle connection
void sig_int(int);           // SIGINT handler
void* thread_main(void*);    // thread main function
void thread_make(int);       // create sub-thread in thread pool
ssize_t written(int, const void*, size_t);  // write buf content to socket

int main(int argc,char* argv[])
{
    int i, navail, maxfd, nsel, connfd, rc;
    socklen_t clilen;
    struct sockaddr_in servaddr;    // host info of server
    struct sockaddr_in cliaddr;     // host info of client
    const int ON = 1;               // value to enable SOL_SOCKET

    // initialize server socket
    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &ON, sizeof(ON));
    bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    listen(listenfd, LISTENQ);

    // initialize thread pool
    nthreads = 10;           // number of threads in the pool
    tptr = calloc(nthreads, sizeof(Thread));
    iget = iput = 0;
    for (i = 0; i < nthreads; ++i)
    {
        thread_make(i);
    }

    // register signal SIGINT
    signal(SIGINT, sig_int);

    // wait for client connection
    for (;;)
    {
        connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
        echo("[SERVICE] accept a connect from %s.\n", inet_ntoa(cliaddr.sin_addr));
        pthread_mutex_lock(&clifd_mutex);   // protect clifd[] and iput/iget
        //echo("[SERVER] get lock, thread = [main]\n");
        clifd[iput] = connfd;               // iput point to next idle position
        if (++iput == MAXCLIENT)
            iput = 0;
        if (iput == iget)                   // the queue overflowed
            errexit("[SERVER] Error: too much clients !");
        pthread_cond_signal(&clifd_cond);
        pthread_mutex_unlock(&clifd_mutex);
    }

    return 0;
}

void thread_make(int i)
{
    pthread_create(&tptr[i].thread_tid, NULL, &thread_main, (void*)i);
}

void* thread_main(void* arg)
{
    int connfd;
    int t_id = (int)arg;
    echo("[SERVER] thread %d strating..\n", t_id);
    for (;;)
    {
        pthread_mutex_lock(&clifd_mutex);
        //echo("[SERVER] get lock, thread = [%d]\n", t_id);
        while (iget == iput)                // the connection queue is empty
            pthread_cond_wait(&clifd_cond, &clifd_mutex);
        //echo("[SERVER] thread [%d] wake up.\n", t_id);
        connfd = clifd[iget];
        if (++iget == MAXCLIENT)
            iget = 0;
        pthread_mutex_unlock(&clifd_mutex);

        tptr[t_id].thread_count++;          // increase task count
        web_child(connfd);                  // handle the request
        close(connfd);                      // close this connection
    }
}

void sig_int(int signo)
{
    int i;
    pr_cpu_time();
    for (i = 0; i < nthreads; ++i)
    {
        echo("[SERVER] thread %d, %ld connections\n", i, tptr[i].thread_count);
    }
    exit(0);
}

void pr_cpu_time(void)
{
    double user, sys;
    struct rusage myusage, childusage;
    if (getrusage(RUSAGE_SELF, &myusage) < 0)
    {
        echo("[SERVER] Error: getrusage(RUSAGE_SELF) failed, %s\n", strerror(errno));
        return;
    }
    if (getrusage(RUSAGE_CHILDREN, &childusage) < 0)
    {
        echo("[SERVER] Error: getrusage(RUSAGE_CHILDREN) failed, %s\n", strerror(errno));
        return;
    }
    user = (double)myusage.ru_utime.tv_sec + myusage.ru_utime.tv_usec/1000000.0;
    user += (double)childusage.ru_utime.tv_sec + childusage.ru_utime.tv_usec/1000000.0;
    sys = (double)myusage.ru_stime.tv_sec + myusage.ru_stime.tv_usec/1000000.0;
    sys += (double)childusage.ru_stime.tv_sec + childusage.ru_stime.tv_usec/1000000.0;
    echo("[SERVER] user time=%g, sys time=%g\n", user, sys);  // show total user time and system time
}

void web_child(int sockfd)
{
    int ntowrite, n;
    ssize_t nread;
    char line[MAXLINE], result[MAXBTYE];  // result array contain random charactors
    for (;;)
    {
        if ((nread = recv(sockfd, line, MAXLINE, 0)) == 0)
        {
            echo("[SERVER] connection closed by client.\n");
            return;
        }
        echo("[SERVER] received from client [%s].\n", line);
        ntowrite = atol(line);
        if ((ntowrite <= 0) || (ntowrite > MAXBTYE))
        {
            echo("[SERVER] client request for %d bytes.\n", ntowrite);
            return;
        }
        //echo("[SERVER] send to client [%s].\n", result);
        if ((n = written(sockfd, result, ntowrite)) != ntowrite)
            echo("[SERVER] Error: expect write %d bytes, actually write %d bytes.\n", ntowrite, n);
        //echo("[SERVER] Debug: wrote %d bytes to client.\n", n);
    }
}

ssize_t written(int fd, const void* vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char* ptr;
    ptr = vptr;
    nleft = n;
    while (nleft > 0)
    {
        // send as much as possible each time from buf, until reach n bytes
        if ((nwritten = send(fd, ptr, nleft, 0)) <= 0)
        {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return (n - nleft);
}

