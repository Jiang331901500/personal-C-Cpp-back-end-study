#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define MAX_PORT		100

#define MAX_BUFFER_SIZE 1024
struct sockitem
{
	int sockfd;
	int (*callback)(int fd, int events, void *arg);
    int epfd;

    char recvbuffer[MAX_BUFFER_SIZE]; // 接收缓冲
	char sendbuffer[MAX_BUFFER_SIZE]; // 发送缓冲
    int recvlength; // 接收缓冲区中的数据长度
    int sendlength; // 发送缓冲区中的数据长度
};

static int client_cnt = 0;  // 统计连接数

#define MAX_EVENTS_NUM (1024*1024)  // 100W个事件同时监听
struct reactor
{
    int epfd;
    struct epoll_event events[MAX_EVENTS_NUM];
};

int recv_cb(int fd, int events, void *arg);

/* 写IO回调函数 */
int send_cb(int fd, int events, void *arg)
{
    struct sockitem *si = arg;
    struct epoll_event ev;

    int clientfd = si->sockfd;
    
    /* 写回的数据此处先简单处理 */
    int ret = send(clientfd, si->sendbuffer, si->sendlength, 0);

    si->callback = recv_cb; // 切回读的回调
    ev.events = EPOLLIN;
    ev.data.ptr = si;
    epoll_ctl(si->epfd, EPOLL_CTL_MOD, si->sockfd, &ev);

    return ret;
}

/* 读IO回调函数 */
int recv_cb(int fd, int events, void *arg)
{
    struct sockitem *si = arg;
    struct epoll_event ev;

    int clientfd = si->sockfd;
    int ret = recv(clientfd, si->recvbuffer, MAX_BUFFER_SIZE, 0);
    if(ret <= 0)
    {
        if(ret < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {   // 被打断直接返回的情况
                return ret;
            }
			printf("# client err... [%d]\n", --client_cnt);
        }
        else
        {
            printf("# client disconn... [%d]\n", --client_cnt);
        }
        
        /* 将当前客户端socket从epoll中删除 */
        ev.events = EPOLLIN;
        ev.data.ptr = si;
        epoll_ctl(si->epfd, EPOLL_CTL_DEL, clientfd, &ev);  
		close(clientfd);
        free(si);
    }
    else
    {
        //printf("# recv data from fd:%d : %s , len = %d\n", clientfd, si->recvbuffer, ret);
        si->recvlength = ret;
        memcpy(si->sendbuffer, si->recvbuffer, si->recvlength); // 将接收到的数据拷贝的发送缓冲区
        si->sendlength = si->recvlength;

        si->callback = send_cb;     // 回调函数要切换成写回调
        struct epoll_event ev;
        ev.events = EPOLLOUT | EPOLLET; // 写的时候最好还是用ET
        ev.data.ptr = si;
        epoll_ctl(si->epfd, EPOLL_CTL_MOD, si->sockfd, &ev);
    }

    return ret;
}

/* accept也属于读IO操作的回调 */
int accept_cb(int fd, int events, void *arg)
{
    struct sockitem *si = arg;
    struct epoll_event ev;

    struct sockaddr_in client;
    memset(&client, 0, sizeof(struct sockaddr_in));
    socklen_t caddr_len = sizeof(struct sockaddr_in);

    int clientfd = accept(si->sockfd, (struct sockaddr*)&client, &caddr_len);
    if(clientfd < 0)
    {
        printf("# accept error\n");
        return clientfd;
    }

    char str[INET_ADDRSTRLEN] = {0};
    printf("recv from %s:%d [%d]\n", inet_ntop(AF_INET, &client.sin_addr, str, sizeof(str)),
        ntohs(client.sin_port), ++client_cnt);

    struct sockitem *client_si = (struct sockitem*)malloc(sizeof(struct sockitem));
    client_si->sockfd = clientfd;
    client_si->callback = recv_cb;  // accept完的下一步就是接收客户端数据
    client_si->epfd = si->epfd;

    memset(&ev, 0, sizeof(struct epoll_event));
    ev.events = EPOLLIN;
    ev.data.ptr = client_si;
    epoll_ctl(si->epfd, EPOLL_CTL_ADD, clientfd, &ev);  // 把客户端socket增加到epoll中监听

    return clientfd;
}

int init_port_and_listen(int port, int epfd)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
		return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if(bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0)
        return -2;

    if(listen(sockfd, 5) < 0)
        return -3;

    /**** 监听socket加入epoll ****/
    struct sockitem *si = (struct sockitem*)malloc(sizeof(struct sockitem));    // 自定义数据，用于传递给回调函数
    si->sockfd = sockfd;
    si->callback = accept_cb;
    si->epfd = epfd; // sockitem 中增加一个epfd成员以便回调函数中使用

    struct epoll_event ev;
    memset(&ev, 0, sizeof(struct epoll_event));
    ev.events = EPOLLIN;    // 默认LT
    ev.data.ptr = si;

    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);    // 添加事件到epoll

    return sockfd;
}

struct reactor ra;  // 放到全局变量，避免大内存进入栈中

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        return 0;
    }

    int port = atoi(argv[1]);
    int i;
    struct sockitem *si;
    int listenfds[MAX_PORT] = {0};

    /* go epoll */
    ra.epfd = epoll_create(1);

    for(i = 0; i < MAX_PORT; i++)   // 建立100个端口进行监听，并且加入epoll
        listenfds[i] = init_port_and_listen(port + i, ra.epfd);

    while(1)
    {
        int nready = epoll_wait(ra.epfd, ra.events, MAX_EVENTS_NUM, -1);
        if(nready < 0)
        {
            printf("epoll_wait error.\n");
            break;
        }

        int i;
        for(i = 0; i < nready; i++)
        {
            si = ra.events[i].data.ptr;
            if(ra.events[i].events & (EPOLLIN | EPOLLOUT))
            {
                if(si->callback != NULL)
                    si->callback(si->sockfd, ra.events[i].events, si);  // 调用回调函数
            }
        }
    }

    for(i = 0; i < MAX_PORT; i++)
    {
        if(listenfds[i] > 0)
            close(listenfds[i]);
    }
}
