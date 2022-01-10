#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <event2/event.h>

void write_cb(evutil_socket_t fd, short events, void *arg);
void read_cb(evutil_socket_t fd, short events, void *arg) {
    char msg[1024];
    struct event *ev = (struct event*)arg;
    int len = recv(fd, msg, sizeof(msg) - 1, 0);
    if(len <= 0) {
        printf("client disconnect or error.\n");
        event_free(ev);
        close(fd);
        return ;
    }

    msg[len] = '\0';
    printf("recv msg: %s\n", msg);
    event_del(ev);
    event_assign(ev, event_get_base(ev), fd, EV_WRITE | EV_PERSIST, write_cb, ev);
    event_add(ev, NULL);
}

void write_cb(evutil_socket_t fd, short events, void *arg) {
    struct event *ev = (struct event*)arg;
    char* replay = "msg received!";
    send(fd, replay, strlen(replay), 0);
    event_del(ev);
    event_assign(ev, event_get_base(ev), fd, EV_READ | EV_PERSIST, read_cb, ev);
    event_add(ev, NULL);
}

void accept_cb(evutil_socket_t fd, short events, void *arg) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    evutil_socket_t clientfd = accept(fd, (struct sockaddr*)&addr, &len);
    evutil_make_socket_nonblocking(clientfd);

    printf("accept a client from %s\n", inet_ntoa(addr.sin_addr));

    // 注册客户端的读事件
    struct event_base* base = (struct event_base*)arg;
    struct event* ev = event_new(NULL, -1, 0, NULL, NULL);
    event_assign(ev, base, clientfd, EV_READ | EV_PERSIST, read_cb, ev);
    event_add(ev, NULL);    // 注册这个客户端的读事件
}

int socket_listen(int port) {
    evutil_socket_t listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0) {
        return -1;
    }

    evutil_make_listen_socket_reuseable(listenfd);

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    if(bind(listenfd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        evutil_closesocket(listenfd);
        return -1;
    }

    if (listen(listenfd, 5) < 0) {
        evutil_closesocket(listenfd);
        return -1;
    }

    evutil_make_socket_nonblocking(listenfd);

    return listenfd;
}

int main() {
    int listenfd = socket_listen(8888);

    // 开始使用 libevent
    struct event_base* base = event_base_new();

    struct event* ev_listen = event_new(base, listenfd, EV_READ | EV_PERSIST, accept_cb, base);

    event_add(ev_listen, NULL); // 注册事件，如果不需要设置超时时间这第二个参数为 NULL

    event_base_dispatch(base);  // 开启主循环

    event_free(ev_listen);
    event_base_free(base);
    return 0;
}