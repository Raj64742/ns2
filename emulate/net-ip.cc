/*-
 * Copyright (c) 1993-1994, 1998
 * The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and the Network Research Group at
 *      Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/emulate/net-ip.cc,v 1.8 1998/02/28 02:44:07 kfall Exp $ (LBL)";
#endif

#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <time.h>
#include <errno.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#define close closesocket
#else
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
typedef int Socket;
#endif
#if defined(sun) && defined(__svr4__)
#include <sys/systeminfo.h>
#endif

#include "config.h"
#include "net.h"
#include "inet.h"
#include "tclcl.h"

//
// IPNetwork: a low-level (raw) IP network object
//

class IPNetwork : public Network {
    public:
	IPNetwork();

	void reset();
	int command(int argc, const char*const* argv);	// virtual in Network
	inline Socket rchannel() { return(rsock_); }	// virtual in Network
	inline Socket schannel() { return(ssock_); }	// virtual in Network

        int send(u_char* buf, int len);			// virtual in Network
        int recv(u_char* buf, int len, sockaddr& from);	// virtual in Network

        inline in_addr& laddr() { return (localaddr_); }
        inline in_addr& dstaddr() { return (destaddr_); }

	static int bindsock(Socket, in_addr&, u_int16_t, sockaddr_in&);
	static int connectsock(Socket, in_addr&, u_int16_t, sockaddr_in&);
	static int rbufsize(Socket, int);
	static int sbufsize(Socket, int);

    protected:

	in_addr destaddr_;		// remote side, if set (network order)
	in_addr localaddr_;		// local side (network order)
	Socket rsock_;
	Socket ssock_;

	int open(int mode);
	int close();
	static void getaddr(Socket, sockaddr_in*);	// sock -> addr discovery
	time_t last_reset_;
};

class UDPIPNetwork : public IPNetwork {
public:
	void reset();
	int add_membership();
	void drop_membership();
        inline int ttl() const { return (ttl_); }
        inline int noloopback_broken() const {
		return (noloopback_broken_);
	}
	int send(u_char*, int);
	int recv(u_char*, int, sockaddr&);
	int open(in_addr&, u_int16_t, int);
	int open(u_int16_t);
	int command(int argc, const char*const* argv);
protected:
	int msetup(Socket, int ttl);
	int openssock(in_addr& addr, u_int16_t port, int ttl);
	int openrsock(in_addr& addr, u_int16_t port, sockaddr_in& local);
        u_int16_t lport_;	// local port (network order)
        u_int16_t port_;	// remote (dst) port (network order)
        int ttl_;
        int noloopback_broken_;
};

static class IPNetworkClass : public TclClass {
    public:
	IPNetworkClass() : TclClass("Network/IP") {}
	TclObject* create(int, const char*const*) {
		return (new IPNetwork);
	}
} nm_ip;

static class UDPIPNetworkClass : public TclClass {
    public:
	UDPIPNetworkClass() : TclClass("Network/IP/UDP") {}
	TclObject* create(int, const char*const*) {
		return (new UDPIPNetwork);
	}
} nm_ip_udp;

IPNetwork::IPNetwork() :
        rsock_(-1), 
        ssock_(-1)
{
	localaddr_.s_addr = 0L;
	destaddr_.s_addr = 0L;
}

UDPIPNetwork::UDPIPNetwork() :
        lport_(0), 
        port_(0),
        ttl_(0),  
        noloopback_broken_(0)
{
}

int
UDPIPNetwork::send(u_char* buf, int len)
{
	int cc = ::send(ssock_, (char*)buf, len, 0);
	if (cc < 0) {
		switch (errno) {
		case ECONNREFUSED:
			/* no one listening at some site - ignore */
#if defined(__osf__) || defined(_AIX) || defined(__FreeBSD__)
			/*
			 * Due to a bug in kern/uipc_socket.c, on several
			 * systems, datagram sockets incorrectly persist
			 * in an error state on receipt of an ICMP
			 * port-unreachable.  This causes unicast connection
			 * rendezvous problems, and worse, multicast
			 * transmission problems because several systems
			 * incorrectly send port unreachables for 
			 * multicast destinations.  Our work around
			 * is to simply close and reopen the socket
			 * (by calling reset() below).
			 *
			 * This bug originated at CSRG in Berkeley
			 * and was present in the BSD Reno networking
			 * code release.  It has since been fixed
			 * in 4.4BSD and OSF-3.x.  It is know to remain
			 * in AIX-4.1.3.
			 *
			 * A fix is to change the following lines from
			 * kern/uipc_socket.c:
			 *
			 *	if (so_serror)
			 *		snderr(so->so_error);
			 *
			 * to:
			 *
			 *	if (so->so_error) {
			 * 		error = so->so_error;
			 *		so->so_error = 0;
			 *		splx(s);
			 *		goto release;
			 *	}
			 *
			 */
			reset();
#endif
			break;

		case ENETUNREACH:
		case EHOSTUNREACH:
			/*
			 * These "errors" are totally meaningless.
			 * There is some broken host sending
			 * icmp unreachables for multicast destinations.
			 * UDP probably aborted the send because of them --
			 * try exactly once more.  E.g., the send we
			 * just did cleared the errno for the previous
			 * icmp unreachable, so we should be able to
			 * send now.
			 */
			cc = ::send(ssock_, (char*)buf, len, 0);
			break;

		default:
			perror("send");
			return -1;
		}
	}
	return cc;
}

//
// raw IP network recv()
//
int
IPNetwork::recv(u_char* buf, int len, sockaddr& sa)
{

printf("RAW IPNetwork::recv!!!\n");
abort();
	int fromlen = sizeof(sa);
	int cc = ::recvfrom(rsock_, (char*)buf, len, 0, &sa, &fromlen);
	if (cc < 0) {
		if (errno != EWOULDBLOCK)
			perror("recvfrom");
		return (-1);
	}
	return (cc);
}

//
// we are given a "raw" IP datagram.
// the raw interface appears to want the len and off fields
// in *host* order, so make it this way here
// note also, that it will compute the cksum "for" us... :(
//
int
IPNetwork::send(u_char* buf, int len)
{
	struct ip *ip = (struct ip*) buf;
	ip->ip_len = ntohs(ip->ip_len);
	ip->ip_off = ntohs(ip->ip_off);

	return (::send(ssock_, (char*)buf, len, 0));
}

int
UDPIPNetwork::recv(u_char* buf, int len, sockaddr& from)
{
	sockaddr_in sfrom;
	int fromlen = sizeof(sfrom);
	int cc = ::recvfrom(rsock_, (char*)buf, len, 0,
			    (sockaddr*)&sfrom, &fromlen);
	if (cc < 0) {
		if (errno != EWOULDBLOCK)
			perror("recvfrom");
		return (-1);
	}
	from = *((sockaddr*)&sfrom);
	if (noloopback_broken_ &&
	    sfrom.sin_addr.s_addr == localaddr_.s_addr &&
	    sfrom.sin_port == lport_) {
		return (0);
	}

	return (cc);
}

int UDPIPNetwork::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		char* cp = tcl.result();
		if (strcmp(argv[1], "port") == 0) {
			sprintf(cp, "%d", ntohs(port_));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ttl") == 0) {
			sprintf(cp, "%d", ttl_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ismulticast") == 0) {
			tcl.result(IN_CLASSD(ntohl(destaddr_.s_addr))? "1" : "0");
			return (TCL_OK);
		}
		if (strcmp(argv[1], "add-membership") == 0) {
			(void)add_membership();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "drop-membership") == 0) {
			drop_membership();
			return (TCL_OK);
		}
	} else if (argc == 3) {

		if (strcmp(argv[1], "loopback") == 0) {
			char c = atoi(argv[2]);
/* XXX why isn't this a problem in mash... */
#ifdef WIN32
#define IP_MULTICAST_LOOP -1
#endif
			if (setsockopt(ssock_, IPPROTO_IP, IP_MULTICAST_LOOP,
				       &c, 1) < 0) {
				/*
				 * If we cannot turn off loopback (Like on the
				 * Microsoft TCP/IP stack), then declare this
				 * option broken so that our packets can be
				 * filtered on the recv path.
				 */
				if (c == 0)
					noloopback_broken_ = 1;
			}
			return (TCL_OK);
		}

		if (strcmp(argv[1], "open") == 0) {
			u_int16_t port = htons(atoi(argv[2]));
			if (open(port) < 0)
				tcl.result("0");
			else
				tcl.result("1");
			return (TCL_OK);
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "open") == 0) {
			in_addr addr;
			addr.s_addr = LookupHostAddr(argv[2]);
			u_int16_t port = htons(atoi(argv[3]));
			int ttl = atoi(argv[4]);
			if (open(addr, port, ttl) < 0)
				tcl.result("0");
			else
				tcl.result("1");
			return (TCL_OK);
		}
	}
	return (IPNetwork::command(argc, argv));
}

int IPNetwork::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "close") == 0) {
			close();
			return (TCL_OK);
		}
		char* cp = tcl.result();
		if (strcmp(argv[1], "destaddr") == 0) {
			strcpy(cp, inet_ntoa(destaddr_));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "localaddr") == 0) {
			strcpy(cp, inet_ntoa(localaddr_));
			return (TCL_OK);
		}
		/* for backward compatability */
		if (strcmp(argv[1], "addr") == 0) {
			strcpy(cp, inet_ntoa(destaddr_));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "interface") == 0) {
			strcpy(cp, inet_ntoa(localaddr_));
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "open") == 0) {
			int mode = parsemode(argv[2]);
			if (open(mode) < 0)
				return (TCL_ERROR);
			return (TCL_OK);
		}
	}
	return (Network::command(argc, argv));
}

int
IPNetwork::open(int mode)
{
	// obtain a raw socket we can use to send ip datagrams
	Socket fd = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
	if (fd < 0) {
		perror("socket(RAW)");
		if (::getuid() != 0 && ::geteuid() != 0) {
			fprintf(stderr,
			    "use of the Network/IP object requires super-user privs\n");
		}

		return (-1);
	}

	// turn on HDRINCL option (we will be writing IP header)
	// in FreeBSD 2.2.5 (and possibly others), the IP id field
	// is set by the kernel routine rip_output()
	// only if it is non-zero, so we should be ok.
	int one = 1;
	if (setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
		perror("setsockopt(IP_HDRINCL)");
		return (-1);
	}

	// sort of curious, but do a connect() even though we have
	// HDRINCL on.  Otherwise, we get ENOTCONN when doing a send()
	sockaddr_in sin;
	in_addr ia = { INADDR_ANY };
	if (connectsock(fd, ia, 0, sin) < 0) {
		perror("connect");
	}
	rsock_ = ssock_ = fd;
	mode_ = mode;
	return 0;
}

//
// open the sending side (open for writing)
//
int
UDPIPNetwork::open(in_addr& addr, u_int16_t port, int ttl)
{
	ssock_ = openssock(addr, port, ttl);
	if (ssock_ < 0)
		return (-1);

	destaddr_ = addr;
	port_ = port;
	ttl_ = ttl;

	/*
	 * Open the receive-side socket.
	 */
	if (add_membership() < 0)
		return (-1);

	last_reset_ = 0;
	return (0);
}

int UDPIPNetwork::open(u_int16_t port)
{
	Socket fd;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return (-1);
	}
	rsock_ = fd;

	localaddr_.s_addr = INADDR_ANY;
	port_ = port;
	ttl_ = 0;
	struct sockaddr_in saddr;
	if (bindsock(fd, localaddr_, port, saddr) < 0) {
		perror("bind");
		return (-1);
	}
	last_reset_ = 0;
	return (0);
}

int
IPNetwork::close()
{
	if (ssock_ >= 0) {
		(void)::close(ssock_);
		ssock_ = -1;
	}
	if (rsock_ >= 0) {
		(void)::close(rsock_);
		rsock_ = -1;
	}
	return (0);
}

/*
 * Control group membership by closing the receive socket.
 * There are too many potential problems across different
 * systems in using the add/drop membership ioctls.
 * (besides, group membership should be implicit in the
 * socket multicast address binding and we should leave
 * the igmp group by closing the socket anyway).
 */
void
UDPIPNetwork::drop_membership()
{
	::close(rsock_);
	rsock_ = -1;
}

int
UDPIPNetwork::add_membership()
{
	if (rsock_ >= 0)
		return (0);

	/*
	 * Connecting the send socket also bound the local address.
	 * On a multihomed host we need to bind the receive socket
	 * to the same local address the kernel has chosen to send on.
	 */
	sockaddr_in local;
	getaddr(ssock_, &local);
	rsock_ = openrsock(destaddr_, port_, local);
	if (rsock_ < 0) {
		if (ssock_ >= 0)
			(void)::close(ssock_);
		return (-1);
	}
	localaddr_ = local.sin_addr;
#if defined(sun) && defined(__svr4__)
	/*
	 * gethostname on solaris prior to 2.6 always returns 0 for
	 * udp sockets.  do a horrible kluge that often fails on
	 * multihomed hosts to get the local address (we don't use
	 * this to open the recv sock, only for the 'interface'
	 * tcl command).
	 */
	if (localaddr_ == 0) {
		char myhostname[1024];
		int error;

		error = sysinfo(SI_HOSTNAME, myhostname, sizeof(myhostname));
		if (error == -1) {
			perror("Getting my hostname");
			exit(-1);
		}
		local_ = LookupHostAddr(myhostname);
	}
#endif
	lport_ = local.sin_port;

	return (0);
}

void
IPNetwork::getaddr(Socket s, sockaddr_in* p)
{
	memset((char *)p, 0, sizeof(*p));
	p->sin_family = AF_INET;
	int len = sizeof(*p);
	if (::getsockname(s, (struct sockaddr *)p, &len) < 0) {
		perror("getsockname");
		p->sin_addr.s_addr = 0;
		p->sin_port = 0;
	}
#ifdef WIN32
	if (p->sin_addr.s_addr == 0) {
		char hostname[80];
		struct hostent *hp;

		if (::gethostname(hostname, sizeof(hostname)) >= 0) {
			if ((hp = ::gethostbyname(hostname)) >= 0) {
				p->sin_addr.s_addr = ((struct in_addr *)hp->h_addr)->s_addr;
			}
		}
	}
#endif	
	return;
}

void IPNetwork::reset()
{
	time_t t = time(0);
	int d = int(t - last_reset_);
	if (d > 3) {
		last_reset_ = t;
		if (ssock_ >= 0) {
			(void)::close(ssock_);
			if (open(mode_) < 0) {
				fprintf(stderr,
				  "IPNetwork(%s): couldn't reset\n");
				mode_ = -1;
				return;
			}
		}
	}
}

void UDPIPNetwork::reset()
{
	time_t t = time(0);
	int d = int(t - last_reset_);
	if (d > 3) {
		last_reset_ = t;
		if (ssock_ >= 0) {
			(void)::close(ssock_);
			ssock_ = openssock(destaddr_, port_, ttl_);
		}
	}
}

Socket
UDPIPNetwork::openrsock(in_addr& addr, u_int16_t port, sockaddr_in& local)
{
	Socket fd;

	fd = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		exit(1);
	}

	mode_ = (mode_ == O_WRONLY) ? O_RDWR : mode_;
	nonblock(fd);
	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on,
			sizeof(on)) < 0) {
		perror("SO_REUSEADDR");
	}
#ifdef SO_REUSEPORT
	on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *)&on,
		       sizeof(on)) < 0) {
		perror("SO_REUSEPORT");
		exit(1);
	}
#endif

	struct sockaddr_in sin;

#ifdef IP_ADD_MEMBERSHIP
	if (IN_CLASSD(ntohl(addr.s_addr))) {
		/*
		 * Try to bind the multicast address as the socket
		 * dest address.  On many systems this won't work
		 * so fall back to a destination of INADDR_ANY if
		 * the first bind fails.
		 */
		if (bindsock(fd, addr, port, sin) < 0) {
			struct in_addr any = { INADDR_ANY };
			if (bindsock(fd, any, port, sin) < 0) {
				perror("bind");
				exit(1);
			}
		}
		/* 
		 * XXX This is bogus multicast setup that really
		 * shouldn't have to be done (group membership should be
		 * implicit in the IP class D address, route should contain
		 * ttl & no loopback flag, etc.).  Steve Deering has promised
		 * to fix this for the 4.4bsd release.  We're all waiting
		 * with bated breath.
		 */
		struct ip_mreq mr;

		mr.imr_multiaddr = addr;
		mr.imr_interface.s_addr = INADDR_ANY;
		if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
			       (char *)&mr, sizeof(mr)) < 0) {
			perror("IP_ADD_MEMBERSHIP");
			exit(1);
		}
	} else
#endif
	{
		/*
		 * bind the local host's address to this socket.  If that
		 * fails, another process probably has the addresses bound so
		 * just exit.
		 */
		if (bindsock(fd, local.sin_addr, port, sin) < 0) {
			perror("bind");
			exit(1);
		}
		/*
		 * MS Windows currently doesn't compy with the Internet Host
		 * Requirements standard (RFC-1122) and won't let us include
		 * the source address in the receive socket demux state.
		 * (The consequence of this is that all conversations have
		 * to be assigned a unique local port so the "vat side
		 * conversations" --- initiated by middle-clicking on
		 * the site name --- doesn't work under windows.)
		 */
#ifndef WIN32
		/*
		 * (try to) connect the foreign host's address to this socket.
		 */
		connectsock(fd, addr, 0, sin);
#endif
	}
	/*
	 * XXX don't need this for the session socket.
	 */	
	if (rbufsize(fd, 80*1024) < 0) {
		if (rbufsize(fd, 32*1024) < 0) {
			perror("SO_RCVBUF");
			exit(1);
		}
	}
	return (fd);
}

int
IPNetwork::bindsock(Socket s, in_addr& addr, u_int16_t port, sockaddr_in& sin)
{
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = port;
	sin.sin_addr = addr;
	return(::bind(s, (struct sockaddr *)&sin, sizeof(sin)));
}

int
IPNetwork::connectsock(Socket s, in_addr& addr, u_int16_t port, sockaddr_in& sin)
{
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = port;
	sin.sin_addr = addr;
	return(::connect(s, (struct sockaddr *)&sin, sizeof(sin)));
}
int 
IPNetwork::sbufsize(Socket s, int cnt)
{   
        return(setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&cnt, sizeof(cnt)));
}   

int
IPNetwork::rbufsize(Socket s, int cnt)
{   
        return(setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&cnt, sizeof(cnt)));
}   

int
UDPIPNetwork::msetup(Socket s, int ttl)
{
	int rval = 0;
#ifdef IP_ADD_MEMBERSHIP
	char c;
	/* turn off loopback */
	c = 0;
	if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &c, 1) < 0) {
		/*
		 * If we cannot turn off loopback (Like on the
		 * Microsoft TCP/IP stack), then declare this
		 * option broken so that our packets can be
		 * filtered on the recv path.
		 */
		if (c == 0)
			rval = 1;
	}
	/* set the multicast TTL */
#ifdef WIN32
	u_int t;
#else   
	u_char t;
#endif  
	t = (ttl > 255) ? 255 : (ttl < 0) ? 0 : ttl;
	if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL,
		       (char*)&t, sizeof(t)) < 0) {
		perror("IP_MULTICAST_TTL");
		return (-1);
	}
#else	/* IP_ADD_MEMBERSHIP NOT DEFINED */
	fprintf(stderr, "\
not compiled with support for IP multicast\n\
you must specify a unicast destination\n");
	return (-1);
#endif  
	return(0);
}

Socket
UDPIPNetwork::openssock(in_addr& addr, u_short port, int ttl)
{
	Socket fd = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		exit(1);
	}

	mode_ = (mode_ == O_RDONLY) ? O_RDWR : mode_;

	nonblock(fd);

	struct sockaddr_in sin;
	static struct in_addr any = { INADDR_ANY };
#ifdef WIN32
	bindsock(fd, any, 0, sin);
#endif
	connectsock(fd, addr, port, sin);

	if (IN_CLASSD(ntohl(addr.s_addr))) {
		int ms = msetup(fd, ttl);
		if (ms < 0)
			exit(1);
		noloopback_broken_ = ms;
	}

	if (sbufsize(fd, 80*1024) < 0) {
		if (sbufsize(fd, 48*1024) < 0) {
			perror("set sock buf send size");
		}
	}
	return (fd);
}
