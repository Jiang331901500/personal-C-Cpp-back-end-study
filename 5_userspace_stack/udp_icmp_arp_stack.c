#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/poll.h>
#include <arpa/inet.h>

#define NETMAP_WITH_LIBS    // 使用netmap必须加上
#include <net/netmap_user.h> 


#define SELF_IP  "192.168.131.129"      // 本机的IP
#define SELF_MAC "00:0c:29:ef:e2:3b"    // 本机的MAC

typedef unsigned char _u8;
typedef unsigned short _u16;
typedef unsigned int _u32;
#pragma pack(1) // 告诉编译器以一个字节对齐

#define ETH_LEN 6
#define PROTO_IP	0x0800
#define PROTO_ARP	0x0806
struct eth_header
{
    _u8 dst_mac[ETH_LEN];
    _u8 src_mac[ETH_LEN];
    _u16 proto;
};

#define IP_LEN 4
enum arp_op {
	arp_op_request = 1,
	arp_op_reply = 2,
};
struct arp_header
{
    _u16 hw_type;
    _u16 proto_type;
    _u8 hw_addr_len;
    _u8 proto_addr_len;
    _u16 op;
    _u8 src_mac[ETH_LEN];
    _u32 src_ip;
    _u8 dst_mac[ETH_LEN];
    _u32 dst_ip;
};

struct arp_packet
{
    struct eth_header eth;
    struct arp_header arp;
};


struct ip_header
{
    _u8 header_len : 4, // 在大端字节序下，首部长度是低4位，版本是高4位
        version : 4;
    _u8 tos;
    _u16 total_len;
    _u16 id;
    _u16 flag_off;
    _u8 ttl;
    _u8 proto;
    _u16 header_check;
    _u32 src_ip;
    _u32 dst_ip;
};

struct ip_packet
{
    struct eth_header eth;
    struct ip_header ip;
};

struct udp_header
{
    _u16 src_port;
    _u16 dst_port;
    _u16 length;
    _u16 check;
};


struct udp_packet
{
    struct eth_header eth;
    struct ip_header ip;
    struct udp_header udp;
    _u8 payload[0]; // 柔性数组
};

struct icmp_header
{
    _u8 type;
    _u8 code;
    _u16 checkSum;
};

struct icmp_packet
{
    struct eth_header eth;
    struct ip_header ip; 
    struct icmp_header icmp;
};

struct icmp_ping_header
{
    struct icmp_header icmp;
    _u16 identifier;
    _u16 seq;
    _u8 data[0];
};

struct icmp_ping_packet
{
    struct eth_header eth;
    struct ip_header ip; 
    struct icmp_ping_header icmp_ping;
};

struct tcp_header
{
    _u16 src_port;
    _u16 dst_port;
    _u32 seq_num;
    _u32 ack_num;
    _u16 res1 : 4,
         header_len : 4,
         fin : 1,
         syn : 1,
         rst : 1,
         psh : 1,
         ack : 1,
         urg : 1,
         res2 : 2;
    _u16 win_size;
    _u16 check;
    _u16 urg_ptr;
};

struct tcp_packet
{
    struct eth_header eth;
    struct ip_header ip;
    struct tcp_header tcp;
    _u8 option[0];
    _u8 payload[0];
};

_u16 in_cksum(_u16 *addr, int len)
{
	register int nleft = len;
	register _u16 *w = addr;
	register int sum = 0;
	_u16 answer = 0;

	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) {
		*(_u8*)(&answer) = *(_u8*)w ;
		sum += answer;
	}

	sum = (sum >> 16) + (sum & 0xffff);	
	sum += (sum >> 16);			
	answer = ~sum;
	
	return answer;
}

void print_mac(_u8* mac)
{
    int i;
    for(i = 0; i < ETH_LEN - 1; i++)
    {
        printf("%02x:", mac[i]);
    }

    printf("%02x", mac[i]);
}

void print_ip(_u32 ip)
{
    _u8* p = (_u8*)&ip;
    int i;
    for(i = 0; i < IP_LEN - 1; i++)
    {
        printf("%d.", p[i]);
    }

    printf("%d", p[i]);
}

/* 将字符串表示形式的MAC地址转换成二进制格式 */
static int str2mac(_u8* mac, char* macstr)
{
    if(macstr == NULL || mac == NULL)
        return -1;

    char* p = macstr;
    int idx = 0;
    _u8 val = 0;

    while(*p != '\0' && idx < ETH_LEN)
    {
        if(*p != ':')
        {
            char c = *p;
            if(c >= 'a' && c <= 'f')
                val = (val << 4) + (c - 'a' + 10);
            else if(c >= 'A' && c <= 'F')
                val = (val << 4) + (c - 'A' + 10);
            else if(c >= '0' && c <= '9')   // 数字0~9
                val = (val << 4) + (c - '0');
            else
                return -1; // 非法字符
        }
        else    // 读到一个字节
        {
            mac[idx++] = val;
            val = 0;
        }

        p++;
    }
    if(idx < ETH_LEN)
        mac[idx] = val; // 最后一个字节
    else
        return -1; // 字节数不对

    return 0;
}


static int udp_process(_u8* stream)
{
    struct udp_packet* udp = (struct udp_packet*)stream;  // 直接取出整个UDP协议下三层的所有首部
    
    int length = ntohs(udp->udp.length);    // 整个UDP包的长度，包含了UDP首部
    udp->payload[length - sizeof(struct udp_header)] = '\0';    // 首部之后就是数据，此处在数据末尾加一个'\0'以便于调试时通过printf打印
    printf("udp recv payload: %s\n", udp->payload);

    return 0;
}

static int icmp_process(struct nm_desc *nmr, _u8* stream)
{
    struct icmp_packet* icmp = (struct icmp_packet*)stream;
    _u16 icmp_len = ntohs(icmp->ip.total_len) - icmp->ip.header_len*4;  // ip数据报总长度减去ip首部长度得到ICMP报文长度
    _u16 icmp_datalen = icmp_len - sizeof(struct icmp_ping_header);     // 减掉ICMP首部就是紧跟其后的其他数据长度

    if(icmp->icmp.type == 8)    // 目前只处理ping请求
    {
        // 加个调试打印
        printf("recv ping(icmp_len=%d,datalen=%d) from ", icmp_len, icmp_datalen);print_mac(icmp->eth.src_mac);
        printf("(");print_ip(icmp->ip.src_ip);printf(")\n");

        struct icmp_ping_packet* icmp_ping = (struct icmp_ping_packet*)stream;

        // 由于ICMP报文最后带的数据长度是动态变化的，因此此处动态申请内存
        _u8* icmp_buf = (_u8*)malloc(sizeof(struct icmp_ping_packet) + icmp_datalen);
        if(icmp_buf == NULL)
            return -1;
        
        struct icmp_ping_packet* icmp_ping_ack = (struct icmp_ping_packet*)icmp_buf;
        memcpy(icmp_ping_ack, icmp_ping, sizeof(struct icmp_ping_packet) + icmp_datalen);  // 整个请求包拷贝过来再修改

        icmp_ping_ack->icmp_ping.icmp.code = 0; // 回显代码是0
        icmp_ping_ack->icmp_ping.icmp.type = 0; // 回显类型是0
        icmp_ping_ack->icmp_ping.icmp.checkSum = 0; // 检验和先置位0

        // 源和目的端IP地址互换，调用数据位置并不影响校验和，因此不需要重新计算IP首部校验
        icmp_ping_ack->ip.dst_ip = icmp_ping->ip.src_ip;
        icmp_ping_ack->ip.src_ip = icmp_ping->ip.dst_ip;

        // 源和目的端MAC地址互换
        memcpy(icmp_ping_ack->eth.dst_mac, icmp_ping->eth.src_mac, ETH_LEN);
        memcpy(icmp_ping_ack->eth.src_mac, icmp_ping->eth.dst_mac, ETH_LEN);

        icmp_ping_ack->icmp_ping.icmp.checkSum = in_cksum((_u16*)&icmp_ping_ack->icmp_ping, \
                                                          sizeof(struct icmp_ping_header) + icmp_datalen);
        nm_inject(nmr, icmp_buf, sizeof(struct icmp_ping_packet) + icmp_datalen);

        free(icmp_buf);
    }

    return 0;
}

static int ip_process(struct nm_desc *nmr, _u8* stream)
{
    struct ip_packet* ip = (struct ip_packet*)stream;   // 取 eth+ip 首部

    switch (ip->ip.proto)
    {
    case IPPROTO_UDP:
        udp_process(stream);
        break;

    case IPPROTO_ICMP:
        icmp_process(nmr, stream);
        break;
    
    default:
        break;
    }

    return 0;
}

static int arp_process(struct nm_desc *nmr, _u8* stream)
{
    struct arp_packet* arp = (struct arp_packet*)stream;
    _u16 op = ntohs(arp->arp.op);

    // 调试打印
    printf("recv arp(op:%d) from ", op);
    print_mac(arp->arp.src_mac);
    printf(" (");
    print_ip(arp->arp.src_ip);
    printf(") to (");
    print_ip(arp->arp.dst_ip);
    printf(")\n");

    _u32 localip = inet_addr(SELF_IP);  // ip地址由字符串转为二进制
    _u8 localmac[ETH_LEN];
    // ARP包中的目的PI地址与本机的IP地址是否一致
    if(arp->arp.dst_ip != localip)  
    {
        return -1;
    }
    str2mac(localmac, SELF_MAC);  // mac地址由字符串转为二进制
    printf("confirmed arp to me: ");
    print_mac(localmac);
    printf(" (");
    print_ip(localip);
    printf(")\n");

    struct arp_packet arp_ack = {0};
    if(op == arp_op_request) // ARP请求
    {
        memcpy(&arp_ack, arp, sizeof(struct arp_packet));

        memcpy(arp_ack.arp.dst_mac, arp->arp.src_mac, ETH_LEN); // arp报文填入目的 mac
        arp_ack.arp.dst_ip = arp->arp.src_ip;                   // arp报文填入目的 ip
        memcpy(arp_ack.eth.dst_mac, arp->arp.src_mac, ETH_LEN); // 以太网首部填入目的 mac

        memcpy(arp_ack.arp.src_mac, localmac, ETH_LEN); // arp报文填入发送端 mac
        arp_ack.arp.src_ip = localip;                   // arp报文填入发送端 ip
        memcpy(arp_ack.eth.src_mac, localmac, ETH_LEN); // 以太网首部填入源 mac

        arp_ack.arp.op = htons(arp_op_reply);  // ARP响应
    }
    else   
    {   // 其他op暂时未实现
        printf("op not implemented.\n");
        return -1;
    }

    nm_inject(nmr, &arp_ack, sizeof(struct arp_packet));    // 发送一个数据包

    return 0;
}

int main()
{
    struct nm_desc *nmr = nm_open("netmap:eth1", NULL, 0, NULL);
    if(nmr == NULL)
        return -1;

    struct pollfd pfd = {0};
    pfd.fd = nmr->fd;   // 这个fd实际指向/dev/netmap
    pfd.events = POLLIN;

    while (1)
    {
        int ret = poll(&pfd, 1, -1);
        if(ret < 0) continue;

        if(pfd.revents & POLLIN)
        {
            struct nm_pkthdr nmhead = {0};
            _u8* stream = nm_nextpkt(nmr, &nmhead);  // 取一个数据包

            struct eth_header* eh = (struct eth_header*)stream;  // 先取出链路层首部
            _u16 proto = ntohs(eh->proto);
            if(proto == PROTO_IP)    // 验证链路层的协议字段
                ip_process(nmr, stream);  // IP协议处理
            else if(proto == PROTO_ARP)
                arp_process(nmr, stream); // ARP协议处理
            else
                printf("error: unknown protocal.\n");
        }
    }
    
    nm_close(nmr);

    return 0;
}