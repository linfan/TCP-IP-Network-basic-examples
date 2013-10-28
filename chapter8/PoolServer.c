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
// struct sockaddr_in
#include <arpa/inet.h>      
#include <pthread.h>

#define PORT      10011
#define LISTENQ   1024
// Max received message bytes
#define MAXLINE   10        
// Max reply message bytes
#define MAXBTYE   4096      
// Max accepted client at the same time
#define MAXCLIENT 32        

typedef struct 
{
    // thread id
    pthread_t thread_tid;   
    // connected client count
    long thread_count;      
} Thread;

// thread array (thread pool)
Thread *tptr;               
// client socket descriptor array
int clifd[MAXCLIENT], iget, iput;   
// socket descriptor in main thread
int listenfd;               
// total thread count
int nthreads;               
pthread_mutex_t clifd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clifd_cond = PTHREAD_COND_INITIALIZER;

// print user and system cpu time
void pr_cpu_time(void);     
// handle connection
void web_child(int);        
// SIGINT handler
void sig_int(int);          
// thread main function
void* thread_main(void*);   
// create sub-thread in thread pool
void thread_make(int);      
// write buf content to socket
ssize_t written(int, const void*, size_t); 

int main(int argc,char* argv[])
{
    int i, navail, maxfd, nsel, connfd, rc;
    socklen_t clilen;
    // host info of server
    struct sockaddr_in servaddr;   
    // host info of client
    struct sockaddr_in cliaddr;    
    // value to enable SOL_SOCKET
    const int ON = 1;              

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
    // number of threads in the pool
    nthreads = 10;          
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
        // protect clifd[] and iput/iget
        pthread_mutex_lock(&clifd_mutex);  
        //echo("[SERVER] get lock, thread = [main]\n");
        // iput point to next idle position
        clifd[iput] = connfd;              
        if (++iput == MAXCLIENT)
            iput = 0;
        // the queue overflowed
        if (iput == iget)                  
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
        // the connection queue is empty
        while (iget == iput)               
            pthread_cond_wait(&clifd_cond, &clifd_mutex);
        //echo("[SERVER] thread [%d] wake up.\n", t_id);
        connfd = clifd[iget];
        if (++iget == MAXCLIENT)
            iget = 0;
        pthread_mutex_unlock(&clifd_mutex);

        // increase task count
        tptr[t_id].thread_count++;         
        // handle the request
        web_child(connfd);                 
        // close this connection
        close(connfd);                     
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
    // show total user time and system time
    echo("[SERVER] user time=%g, sys time=%g\n", user, sys); 
}

void web_child(int sockfd)
{
    int ntowrite, n;
    ssize_t nread;
    // result array contain random charactors
    char line[MAXLINE], result[MAXBTYE]; 
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

