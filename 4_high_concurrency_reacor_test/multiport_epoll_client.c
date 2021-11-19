#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define MAX_CONNECTION  340000  // 单个客户端建立34W个连接，3个客户端就有100W
#define MAX_BUFSIZE		128
#define MAX_EPOLLSIZE	(384*1024)
#define MAX_PORT		100     // 最大端口数量

#define TIME_MS_USED(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)  // 用于计算耗时

/* 设置fd为非阻塞 */
static int fdSetNonBlock(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL);
    if(flags < 0) return flags;

    flags |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flags) < 0) return -1;

    return 0;
}

/* 设置socket SO_REUSEADDR*/
static int sockSetReuseAddr(int sockfd)
{
    int reuse = 1;
    return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
}

struct epoll_event events[MAX_EPOLLSIZE];
int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        printf("Usage: %s ip port", argv[0]);
        return 0;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int connections = 0;    // 建立连接的计数，用于统计
    char buffer[MAX_BUFSIZE] = {0};
    int i;
    int clientFinishTest = 0;
    int portOffset = 0;
    int epfd = epoll_create(1);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);

    struct timeval loopBegin, loopEnd;
    gettimeofday(&loopBegin, NULL);

    while(1)
    {
        struct epoll_event ev;
		int sockfd = 0;

        if(connections < MAX_CONNECTION)    // 连接数量未达到最大值就一直新建连接并connect到服务端
        {
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if(sockfd < 0)
            {
                perror("socket");
                goto errExit;
            }

            addr.sin_port = htons(port + portOffset);
            portOffset = (portOffset + 1) % MAX_PORT;   // 均匀地使用这100个端口

            if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            {
                perror("connect");
                goto errExit;
            }

            fdSetNonBlock(sockfd);
            sockSetReuseAddr(sockfd);

            ev.events = EPOLLIN | EPOLLOUT;
            ev.data.fd = sockfd;
            epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);    // 当前连接添加到epoll中进行监视

            connections++; // 连接数增加
        }

        // 每增加1000个连接就执行一次
        if(connections % 1000 == 0 || connections == MAX_CONNECTION)
        {
			int nready = epoll_wait(epfd, events, 256, 100);	// 每次处理256个，避免处理太过于频繁
			for(i = 0; i < nready; i++)
			{
				int clientfd = events[i].data.fd;
				if (events[i].events & EPOLLIN) 
				{
					char rBuffer[MAX_BUFSIZE] = {0};				
					ssize_t length = recv(clientfd, rBuffer, MAX_BUFSIZE, 0);
					if (length > 0) 
					{
						printf("# Recv from server:%s\n", rBuffer);
					} 
					else if (length == 0) 
					{
						printf("# Disconnected. clientfd:%d\n", clientfd);
						connections --;
						epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &events[i]);
						close(clientfd);
					} 
					else 
					{
						if (errno == EINTR) continue;

						printf(" Error clientfd:%d, errno:%d\n", clientfd, errno);
						epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &events[i]);
						close(clientfd);
					}
				} 
				else if (events[i].events & EPOLLOUT)
				{
					sprintf(buffer, "# data from %d\n", clientfd);
					send(clientfd, buffer, strlen(buffer), 0);
				}
				else 
				{
					printf(" clientfd:%d, unknown events:%d\n", clientfd, events[i].events);
				}
				usleep(1000);  // 适当停一下，避免服务端虚拟机网络IO压力过大
			}
			
			
            if(!clientFinishTest)   // 当前客户端的测试尚未结束
            {
                /* 计算建立1000个连接需要的时间 */
                gettimeofday(&loopEnd, NULL);
                int msPer1K = TIME_MS_USED(loopEnd, loopBegin);
                gettimeofday(&loopBegin, NULL); // 进入下一轮计数

                printf("-- connections: %d, sockfd:%d, time_used:%d\n", connections, sockfd, msPer1K);
				
				if(connections == MAX_CONNECTION)
				{
					printf("# Client finished the test.\n");
					clientFinishTest = 1;
				}
            }
        }

        usleep(1000);   // 短暂休眠1ms再继续
    }

    return 0;

errExit:
	printf("error : %s\n", strerror(errno));
	return 0;
}