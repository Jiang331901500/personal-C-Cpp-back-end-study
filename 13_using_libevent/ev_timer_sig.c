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
#include <event2/event.h>

void timer_callback(int fd, short events, void* arg) 
{
    time_t now = time(NULL);
    printf("timer_callback. now: %s", (char*)ctime(&now));
    //event_del(timer);
    // struct timeval tv = {1,0};
    // event_add(timer, &tv);
}

void sig_int_callback(int fd, short event, void *arg)
{
    struct event* evsigint = (struct event*)arg;
    printf("received SIGINT\n");    // 终端执行 kill -2 <pid>
    event_del(evsigint);
}

int main()
{
    struct event_base* base = event_base_new();

    struct event* evtimer = event_new(base, -1, EV_PERSIST, timer_callback, base);
    struct timeval tv = {3, 0}; // {秒, 微秒}
    event_add(evtimer, &tv);    // tv 为超时时间

    struct event* evsigint = event_new(base, -1, 0, NULL, NULL);
    event_assign(evsigint, base, SIGINT, EV_SIGNAL, sig_int_callback, evsigint);
    event_add(evsigint, NULL);

    event_base_dispatch(base);

    event_free(evtimer);
    event_free(evsigint);
    event_base_free(base);
    return 0;
}