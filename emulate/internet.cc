#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>   
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>


#include "internet.h"

/*  
 * in_cksum --
 *      Checksum routine for Internet Protocol family headers (C Version)
 *      [taken from ping.c]
 */ 
u_short  
Internet::in_cksum(u_short* addr, int len)
{   
        register int nleft = len;       
        register u_short *w = addr;
        register int sum = 0;
        u_short answer = 0;
    
        /*                      
         * Our algorithm is simple, using a 32 bit accumulator (sum), we add
         * sequential 16 bit words to it, and at the end, fold back all the
         * carry bits from the top 16 bits into the lower 16 bits.
         */      
        while (nleft > 1)  {
                sum += *w++;
                nleft -= 2;
        }       
    
        /* mop up an odd byte, if necessary */
        if (nleft == 1) {
                *(u_char *)(&answer) = *(u_char *)w ;
                sum += answer;
        }

        /* add back carry outs from top 16 bits to low 16 bits */
        sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
        sum += (sum >> 16);                     /* add carry */
        answer = ~sum;                          /* truncate to 16 bits */
        return(answer);
}
#include <netinet/in_systm.h>
#include <netinet/ip.h>
void
Internet::print_ip(ip *ip)
{   
        char buf[64];
        u_short off = ntohs(ip->ip_off);
        printf("IP v:%d, ihl:%d, tos:0x%x, id:%d, off:%d [df:%d, mf:%d], sum:%d, p
rot:%d\n",
                ip->ip_v, ip->ip_hl, ip->ip_tos, ntohs(ip->ip_id),
                off & IP_OFFMASK,
                (off & IP_DF) ? 1 : 0,
                (off & IP_MF) ? 1 : 0,
                ip->ip_sum, ip->ip_p);
	printf("IP src:%s, ", inet_ntoa(ip->ip_src));
	printf("dst: %s\n", inet_ntoa(ip->ip_dst));
        printf("IP len:%d ttl: %d\n",
                ntohs(ip->ip_len), ip->ip_ttl);
}   
