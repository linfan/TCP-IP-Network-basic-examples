#include <sys/socket.h>      // socket interface
#include <sys/epoll.h>       // epoll interface
#include <netinet/in.h>      // struct sockaddr_in
#include <arpa/inet.h>       // IP addr convertion
#include <fcntl.h>           // File descriptor controller
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>          // bzero()
#include <stdlib.h>          // malloc(), free()
#include <errno.h>
extern int errno;

#define MAXBTYE     10       // maximum received data byte
#define OPEN_MAX    100
#define LISTENQ     20
#define SERV_PORT   10012
#define INFTIM      1000
#define LOCAL_ADDR  "127.0.0.1"
#define TIMEOUT     500

struct task                           // task item in thread pool
{
    epoll_data_t data;         // file descriptor or user_data
    struct task* next;                // next task
};

struct user_data                      // for data transporting
{
    int fd;
    unsigned int n_size;              // real received data size
    char line[MAXBTYE];               // content received
};

void *readtask(void *args);
void *writetask(void *args);

int epfd;                             // epoll descriptor from epoll_create()
struct epoll_event ev;                // register epoll_ctl()
struct epoll_event events[LISTENQ];   // store queued events from epoll_wait()
pthread_mutex_t r_mutex;              // mutex lock to protect readhead/readtail
pthread_cond_t r_condl;
pthread_mutex_t w_mutex;              // mutex lock to protect writehead/writetail
pthread_cond_t w_condl;
struct task *readhead = NULL, *readtail = NULL;
struct task *writehead = NULL, *writetail = NULL;

void setnonblocking(int sock)
{
    int opts;
    if ((opts = fcntl(sock, F_GETFL)) < 0)
        errexit("GETFL %d failed", sock);
    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0)
        errexit("SETFL %d failed", sock);
}

int main(int argc,char* argv[])
{
    int i, maxi, nfds;                // nfds is number of events (number of returned fd)
    int listenfd, connfd;
    pthread_t tid1, tid2;             // read task threads
    pthread_t tid3, tid4;             // write back threads
    struct task *new_task = NULL;     // task node
    socklen_t clilen;
    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;

    pthread_mutex_init(&r_mutex, NULL);
    pthread_cond_init(&r_condl, NULL);
    pthread_mutex_init(&w_mutex, NULL);
    pthread_cond_init(&w_condl, NULL);

    // threads for reading thread pool
    pthread_create(&tid1, NULL, readtask, NULL);
    pthread_create(&tid2, NULL, readtask, NULL);
    // threads for respond to client
    pthread_create(&tid3, NULL, writetask, NULL);
    pthread_create(&tid4, NULL, writetask, NULL);

    epfd = epoll_create(256);         // epoll descriptor, for handling accept
    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    setnonblocking(listenfd);         // set the descriptor as non-blocking
    ev.data.fd = listenfd;            // event related descriptor
    ev.events = EPOLLIN | EPOLLET;    // monitor in message, edge trigger
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);     // register epoll event

    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    char *local_addr = LOCAL_ADDR;
    inet_aton(local_addr, &(serveraddr.sin_addr));
    serveraddr.sin_port = htons(SERV_PORT);
    bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    listen(listenfd, LISTENQ);

    maxi = 0;
    for(;;)
    {
        nfds = epoll_wait(epfd, events, LISTENQ, TIMEOUT);      // waiting for epoll event
        for (i = 0; i < nfds; ++i)    // In case of edge trigger, must go over each event
        {
            if (events[i].data.fd == listenfd)    // Get new connection
            {
                // accept the client connection
                connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clilen);
                if (connfd < 0)
                    errexit("connfd < 0");
                setnonblocking(connfd);
                echo("[SERVER] connect from %s \n", inet_ntoa(clientaddr.sin_addr));
                ev.data.fd = connfd;
                ev.events = EPOLLIN | EPOLLET;    // monitor in message, edge trigger
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);    // add fd to epoll queue
            }
            else if (events[i].events & EPOLLIN)  // Received data
            {
                if (events[i].data.fd < 0)
                    continue;
                echo("[SERVER] put task %d to read queue\n", events[i].data.fd);
                new_task = malloc(sizeof(struct task));
                new_task->data.fd = events[i].data.fd;
                new_task->next = NULL;
                //echo("[SERVER] thread %d epollin before lock\n", pthread_self());
                pthread_mutex_lock(&r_mutex);       // protect task queue (readhead/readtail)
                //echo("[SERVER] thread %d epollin after lock\n", pthread_self());
                if (readhead == NULL) // the queue is empty
                {
                    readhead = new_task;
                    readtail = new_task;
                }
                else                  // queue is not empty
                {
                    readtail->next = new_task;
                    readtail = new_task;
                }
                pthread_cond_broadcast(&r_condl);   // trigger readtask thread
                //echo("[SERVER] thread %d epollin before unlock\n", pthread_self());
                pthread_mutex_unlock(&r_mutex);
                //echo("[SERVER] thread %d epollin after unlock\n", pthread_self());
            }
            else if (events[i].events & EPOLLOUT) // Have data to send
            {
                if (events[i].data.ptr == NULL)
                    continue;
                echo("[SERVER] put task %d to write queue\n", ((struct task*)events[i].data.ptr)->data.fd);
                new_task = malloc(sizeof(struct task));
                new_task->data.ptr = (struct user_data*)events[i].data.ptr;
                new_task->next = NULL;
                //echo("[SERVER] thread %d epollout before lock\n", pthread_self());
                pthread_mutex_lock(&w_mutex);
                //echo("[SERVER] thread %d epollout after lock\n", pthread_self());
                if (writehead == NULL) // the queue is empty
                {
                    writehead = new_task;
                    writetail = new_task;
                }
                else                  // queue is not empty
                {
                    writetail->next = new_task;
                    writetail = new_task;
                }
                pthread_cond_broadcast(&w_condl);   // trigger writetask thread
                //echo("[SERVER] thread %d epollout before unlock\n", pthread_self());
                pthread_mutex_unlock(&w_mutex);
                //echo("[SERVER] thread %d epollout after unlock\n", pthread_self());
            }
            else
            {
                echo("[SERVER] Error: unknown epoll event");
            }
        }
    }

    return 0;
}

void *readtask(void *args)
{
    int fd = -1;
    int n, i;
    struct user_data* data = NULL;
    while(1)
    {
        //echo("[SERVER] thread %d readtask before lock\n", pthread_self());
        pthread_mutex_lock(&r_mutex);  // protect task queue (readhead/readtail)
        //echo("[SERVER] thread %d readtask after lock\n", pthread_self());
        while(readhead == NULL)
            pthread_cond_wait(&r_condl, &r_mutex);  // if condl false, will unlock mutex

        fd = readhead->data.fd;
        struct task* tmp = readhead;
        readhead = readhead->next;
        free(tmp);

        //echo("[SERVER] thread %d readtask before unlock\n", pthread_self());
        pthread_mutex_unlock(&r_mutex);
        //echo("[SERVER] thread %d readtask after unlock\n", pthread_self());

        echo("[SERVER] readtask %d handling %d\n", pthread_self(), fd);
        data = malloc(sizeof(struct user_data));
        data->fd = fd;
        if ((n = recv(fd, data->line, MAXBTYE, 0)) < 0)
        {
            if (errno == ECONNRESET)
                close(fd);
            echo("[SERVER] Error: readline failed: %s\n", strerror(errno));
            if (data != NULL)
                free(data);
        }
        else if (n == 0)
        {
            close(fd);
            echo("[SERVER] Error: client closed connection.\n");
            if (data != NULL)
                free(data);
        }
        else
        {
            data->n_size = n;
            for (i = 0; i < n; ++i)
            {
                if (data->line[i] == '\n' || data->line[i] > 128)
                {
                    data->line[i] = '\0';
                    data->n_size = i + 1;
                }
            }
            echo("[SERVER] readtask %d received %d : [%d] %s\n", pthread_self(), fd, data->n_size, data->line);
            if (data->line[0] != '\0')
            {
                // modify monitored event to EPOLLOUT,  wait next loop to send respond
                ev.data.ptr = data;
                ev.events = EPOLLOUT | EPOLLET;     // Modify event to EPOLLOUT
                epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);        // modify moditored fd event
            }
        }
    }
}

void *writetask(void *args)
{
    unsigned int n;
    struct user_data *rdata = NULL;   // data to wirte back to client
    while(1)
    {
        //echo("[SERVER] thread %d writetask before lock\n", pthread_self());
        pthread_mutex_lock(&w_mutex);
        //echo("[SERVER] thread %d writetask after lock\n", pthread_self());
        while(writehead == NULL)
            pthread_cond_wait(&w_condl, &w_mutex);  // if condl false, will unlock mutex

        rdata = (struct user_data*)writehead->data.ptr;
        struct task* tmp = writehead;
        writehead = writehead->next;
        free(tmp);

        //echo("[SERVER] thread %d writetask before unlock\n", pthread_self());
        pthread_mutex_unlock(&w_mutex);
        //echo("[SERVER] thread %d writetask after unlock\n", pthread_self());
        
        echo("[SERVER] writetask %d sending %d : [%d] %s\n", pthread_self(), rdata->fd, rdata->n_size, rdata->line);
        // send responce to client
        if ((n = send(rdata->fd, rdata->line, rdata->n_size, 0)) < 0)
        {
            if (errno == ECONNRESET)
                close(rdata->fd);
            echo("[SERVER] Error: send responce failed: %s\n", strerror(errno));
        }
        else if (n == 0)
        {
            close(rdata->fd);
            echo("[SERVER] Error: client closed connection.");
        }
        else
        {
            // modify monitored event to EPOLLIN, wait next loop to receive data
            ev.data.fd = rdata->fd;
            ev.events = EPOLLIN | EPOLLET;    // monitor in message, edge trigger
            epoll_ctl(epfd, EPOLL_CTL_MOD, rdata->fd, &ev);    // modify moditored fd event
        }
        free(rdata);         // delete data
    }
}

