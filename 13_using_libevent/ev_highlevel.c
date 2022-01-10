#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

// socket触发读事件时的回调
void socket_read_callback(struct bufferevent *bev, void *arg)
{
    // 获取读缓冲区并操作读缓冲中的数据
    struct evbuffer* evbuf = bufferevent_get_input(bev);
    char* msg = evbuffer_readln(evbuf, NULL, EVBUFFER_EOL_LF);  // 从中读一行，需要指定换行符
    if(!msg) return;
    // 也可以直接用 bufferevent_read 读数据
    // bufferevent_read(struct bufferevent *bufev, void *data, size_t size)

    printf("server read data: %s\n", msg);

    char reply[1024] = {0}; // 此处简单地将接收到的数据回传
    snprintf(reply, 1023, "recvieced msg: %s\n", msg);
    free(msg);
    bufferevent_write(bev, reply, strlen(reply));
}

// socket出现如错误、关闭等异常事件时的回调
void socket_event_callback(struct bufferevent *bev, short events, void *arg)
{
    if(events & BEV_EVENT_EOF) {
        printf("fd %d closed connection.\n", bufferevent_getfd(bev));
    }
    else if(events & BEV_EVENT_ERROR) {
        perror("socket ");
    }
    else if(events & BEV_EVENT_TIMEOUT) {
        printf("socket timeout.\n");
    }
    bufferevent_free(bev);  // 此处关闭并释放 socket
}

void listener_callback(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sock, int socklen, void *arg)
{
    char ip[INET_ADDRSTRLEN + 1] = {0};
    evutil_inet_ntop(AF_INET, sock, ip, sizeof(ip) - 1);
    printf("accept client from %s:%d\n", ip, ((struct sockaddr_in*)sock)->sin_port);
    struct event_base* base = (struct event_base*)arg;

    struct bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE); // 新建一个 socket bev
    bufferevent_setcb(bev, socket_read_callback, NULL, socket_event_callback, base); // 设置读、写、以及异常时的回调函数
    bufferevent_enable(bev, EV_READ | EV_PERSIST);   // 使能这个 ev的事件检测
}

// 处理标准输入 IO
void stdio_callback(struct bufferevent *bev, void *arg)
{
    struct evbuffer* evbuf = bufferevent_get_input(bev);
    char* msg = evbuffer_readln(evbuf, NULL, EVBUFFER_EOL_LF);
    if(!msg) return;

    if (strcmp(msg, "quit") == 0) {
        printf("server safely exit!!!\n");
        exit(0);
        return;
    }
    printf("stdio read the data: %s\n", msg);
    free(msg);
}

int main()
{
    struct event_base *base = event_base_new();

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(8888);

    /* socket 监听 */
    struct evconnlistener *listener = evconnlistener_new_bind(base, listener_callback, base, LEV_OPT_REUSEABLE_PORT | LEV_OPT_CLOSE_ON_FREE, 10, (struct sockaddr *)&sin, sizeof(struct sockaddr_in));

    /* 普通 fd 的 IO 事件管理，此处以标准输入 stdin 为例 */
    struct bufferevent* iobev = bufferevent_socket_new(base, STDIN_FILENO, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(iobev, stdio_callback, NULL, NULL, base);
    bufferevent_enable(iobev, EV_READ | EV_PERSIST);

    /* 定时事件 */
    

    /* 开启主循环 */
    event_base_dispatch(base);

    /* 结束释放资源 */
    evconnlistener_free(listener);
    bufferevent_free(iobev);
    event_base_free(base);
}