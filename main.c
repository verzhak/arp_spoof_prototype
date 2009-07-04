#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
//#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <netinet/ip.h>
#include <netinet/in.h>

#include <sys/ioctl.h>
//#include <net/if.h>

#define print_error fprintf(stderr,"ERROR: [%s]\n",strerror(errno))

in_addr_t srcIP(in_addr_t dest)
{
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = 0;
	sa.sin_addr.s_addr = dest;

	int sock = socket(AF_INET,SOCK_RAW,IPPROTO_RAW);
	if(sock == -1)
		return -1;

	if(connect(sock,(struct sockaddr*) &sa,sizeof(sa)) == -1)
	{
		close(sock);
		return -1;
	}

	struct sockaddr addr;
	socklen_t len = sizeof(addr);
	if(getsockname(sock,&addr,&len))
	{
		close(sock);
		return -1;
	}

	in_addr_t ret = ((struct sockaddr_in*) & addr)->sin_addr.s_addr;

	close(sock);

	return ret;
}

void aboutInterfaces(int sock)
{
	struct ifconf conf;
	int len = sizeof(struct ifreq) * 11;
	conf.ifc_len = len;
	conf.ifc_req = (struct ifreq*) malloc(len);
	if(ioctl(sock,SIOCGIFCONF,(void*)&conf))
		print_error;
	else
	{
		struct sockaddr_in* sa;
		int num = conf.ifc_len / sizeof(struct ifreq);
		for(num--;num >= 0;num--)
		{
			sa = (struct sockaddr_in *) &conf.ifc_req[num].ifr_addr;
			if(sa->sin_family == AF_INET)
				printf("name = [%s], address = [%s]\n",
					conf.ifc_req[num].ifr_name,
					inet_ntoa(sa->sin_addr));
		}
	}

	free(conf.ifc_req);
}

int getDeviceNum(int sock,const char* deviceName)
{
	struct ifreq req;
	strcpy(req.ifr_name,deviceName);

	if(ioctl(sock,SIOCGIFINDEX,(void*) &req))
		return -1;
	else
		return req.ifr_ifindex;
}

int main()
{
	int sock = socket(AF_PACKET,SOCK_DGRAM,ETH_P_ARP);
	if(sock < 1)
		print_error;

	// ############################################################################ 
	// TODO
	aboutInterfaces(sock);
	in_addr_t a = srcIP(inet_addr("194.67.57.226"));
	if(a == -1)
	{
		print_error;
		return 1;
	}
	struct in_addr b;
	b.s_addr = a;
	printf("0x%X - %s\n",a,inet_ntoa(b));
	a = srcIP(inet_addr("127.0.0.1"));
	if(a == -1)
	{
		print_error;
		return 1;
	}
	b.s_addr = a;
	printf("0x%X - %s\n",a,inet_ntoa(b));
	// ############################################################################ 

	struct sockaddr_ll addr;
	addr.sll_family = AF_PACKET;
	addr.sll_protocol = htons(ETH_P_ARP);

	addr.sll_ifindex = getDeviceNum(sock,"eth0");
	
	if(addr.sll_ifindex == -1)
	{
		print_error;
		close(sock);
		return -1;
	}

	addr.sll_hatype = ARPOP_REQUEST;
	addr.sll_pkttype = PACKET_BROADCAST;
	addr.sll_halen = 6;
	memset((void*)addr.sll_addr,0xFF,8);

	char arpreq_buf[100];
	struct arphdr *arpreq_hdr = (struct arphdr*) arpreq_buf;
	arpreq_hdr->ar_hrd = htons(0x0001); // Ethernet
	arpreq_hdr->ar_pro = htons(0x0800); // IPv4
	arpreq_hdr->ar_hln = 6;
	arpreq_hdr->ar_pln = 4;
	arpreq_hdr->ar_op = htons(ARPOP_REQUEST);

	arpreq_buf[sizeof(struct arphdr) + 0] = 0x00;
	arpreq_buf[sizeof(struct arphdr) + 1] = 0x50;
	arpreq_buf[sizeof(struct arphdr) + 2] = 0x8d;
	arpreq_buf[sizeof(struct arphdr) + 3] = 0x72;
	arpreq_buf[sizeof(struct arphdr) + 4] = 0x8d;
	arpreq_buf[sizeof(struct arphdr) + 5] = 0x21;

	in_addr_t dest = inet_addr("10.4.135.17");

	*((uint32_t*)(arpreq_buf + sizeof(struct arphdr) + 6)) = (uint32_t) srcIP(dest);

	*((uint32_t*)(arpreq_buf + sizeof(struct arphdr) + 6 + 4 + 6)) = dest;

	memset((void*)(arpreq_buf + sizeof(struct arphdr) + ETH_ALEN + 4),0xFF,ETH_ALEN);

	ssize_t size = sendto(sock,arpreq_buf,sizeof(struct arphdr) + 2 * (ETH_ALEN + 4),
			0, (struct sockaddr*) &addr, sizeof(struct sockaddr_ll));

	if(size < 0)
		print_error;
	else
		printf("We send %d byte\n",size);

	close(sock);

	return 0;
}
