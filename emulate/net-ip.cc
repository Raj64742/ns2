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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/emulate/net-ip.cc,v 1.2 1998/01/30 23:38:59 kfall Exp $ (LBL)";
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
#endif
#if defined(sun) && defined(__svr4__)
#include <sys/systeminfo.h>
#endif

#include "config.h"
#include "net.h"
#include "inet.h"
#include "tclcl.h"

class IPNetwork : public Network {
    public:
	IPNetwork();

        void send(u_char* buf, int len);
        int recv(u_char* buf, int len, u_int32_t& from);
	void reset();
	int command(int argc, const char*const* argv);

	inline int rchannel() { return(rsock_); }
	inline int schannel() { return(ssock_); }
        inline int port() const { return (port_); }
        inline int ttl() const { return (ttl_); }
        inline int noloopback_broken() const {
		return (noloopback_broken_);
	}

        inline u_int32_t addr() const { return (addr_); }
        inline u_int32_t interface() const { return (local_); }

    protected:
        u_int32_t addr_;
        u_int32_t local_;
        u_short lport_;
        u_short port_;
	int rsock_;
	int ssock_;
        int ttl_;
        int noloopback_broken_;


	int open(u_int32_t addr, int port, int ttl);
	int open(int port);
	int close();
	void localname(sockaddr_in*);
	int openssock(u_int32_t addr, u_short port, int ttl);
	int openrsock(u_int32_t addr, u_short port,
		      const struct sockaddr_in& local);
	int add_membership();
	void drop_membership();
	time_t last_reset_;
};

static class IPNetworkClass : public TclClass {
    public:
	IPNetworkClass() : TclClass("Network/IP") {}
	TclObject* create(int, const char*const*) {
		return (new IPNetwork);
	}
} nm_ip;

IPNetwork::IPNetwork() :
        addr_(0), 
        local_(0),
        lport_(0), 
        port_(0),
        ttl_(0),  
        rsock_(-1), 
        ssock_(-1),
        noloopback_broken_(0)
{
}

void
IPNetwork::send(u_char* buf, int len)
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
			(void)::send(ssock_, (char*)buf, len, 0);
			break;

		default:
			perror("send");
			return;
		}
	}
}

int
IPNetwork::recv(u_char* buf, int len, u_int32_t& from)
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
	from = sfrom.sin_addr.s_addr;
	if (noloopback_broken_ && from == local_ && sfrom.sin_port == lport_)
		return (0);

	return (cc);
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
		if (strcmp(argv[1], "addr") == 0) {
			strcpy(cp, intoa(addr_));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "interface") == 0) {
			strcpy(cp, intoa(local_));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "port") == 0) {
			sprintf(cp, "%d", ntohs(port_));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ttl") == 0) {
			sprintf(cp, "%d", ttl_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ismulticast") == 0) {
			tcl.result(IN_CLASSD(ntohl(addr_))? "1" : "0");
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
		if (strcmp(argv[1], "open") == 0) {
			int port = htons(atoi(argv[2]));
			if (open(port) < 0)
				tcl.result("0");
			else
				tcl.result("1");
			return (TCL_OK);
		}
			
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
	} else if (argc == 5) {
		if (strcmp(argv[1], "open") == 0) {
			u_int32_t addr = LookupHostAddr(argv[2]);
			int port = htons(atoi(argv[3]));
			int ttl = atoi(argv[4]);
			if (open(addr, port, ttl) < 0)
				tcl.result("0");
			else
				tcl.result("1");
			return (TCL_OK);
		}
	}
	return (Network::command(argc, argv));
}

int IPNetwork::open(u_int32_t addr, int port, int ttl)
{
	addr_ = addr;
	port_ = port;
	ttl_ = ttl;

	ssock_ = openssock(addr, port, ttl);
	if (ssock_ < 0)
		return (-1);

	/*
	 * Open the receive-side socket.
	 */
	if (add_membership() < 0)
		return (-1);

	last_reset_ = 0;
	return (0);
}

/* UDP unicast server */
int IPNetwork::open(int port)
{
	int fd;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return (-1);
	}
	addr_ = INADDR_ANY;
	port_ = port;
	ttl_ = 0;

	struct sockaddr_in saddr;
	bzero((char*)&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port = port;
	if (::bind(fd, (struct sockaddr*)&saddr, sizeof(saddr)) < 0) {
		perror("bind");
		return(-1);
	}
	rsock_ = fd;
	last_reset_ = 0;
	return (0);
}

int IPNetwork::close()
{
	if (ssock_ >= 0) {
		::close(ssock_);
		ssock_ = -1;
	}
	if (rsock_ >= 0) {
		::close(rsock_);
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
void IPNetwork::drop_membership()
{
	::close(rsock_);
	rsock_ = -1;
}

int IPNetwork::add_membership()
{
	if (rsock_ >= 0)
		return (0);

	/*
	 * Connecting the send socket also bound the local address.
	 * On a multihomed host we need to bind the receive socket
	 * to the same local address the kernel has chosen to send on.
	 */
	sockaddr_in local;
	localname(&local);
	rsock_ = openrsock(addr_, port_, local);
	if (rsock_ < 0) {
		if (ssock_ >= 0)
			(void)::close(ssock_);
		return (-1);
	}
	local_ = local.sin_addr.s_addr;
#if defined(sun) && defined(__svr4__)
	/*
	 * gethostname on solaris prior to 2.6 always returns 0 for
	 * udp sockets.  do a horrible kluge that often fails on
	 * multihomed hosts to get the local address (we don't use
	 * this to open the recv sock, only for the 'interface'
	 * tcl command).
	 */
	if (local_ == 0) {
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

void IPNetwork::localname(sockaddr_in* p)
{
	memset((char *)p, 0, sizeof(*p));
	p->sin_family = AF_INET;
	int len = sizeof(*p);
	if (getsockname(ssock_, (struct sockaddr *)p, &len) < 0) {
		perror("getsockname");
		p->sin_addr.s_addr = 0;
		p->sin_port = 0;
	}
#ifdef WIN32
	if (p->sin_addr.s_addr == 0) {
		char hostname[80];
		struct hostent *hp;

		if (gethostname(hostname, sizeof(hostname)) >= 0) {
			if ((hp = gethostbyname(hostname)) >= 0) {
				p->sin_addr.s_addr = ((struct in_addr *)hp->h_addr)->s_addr;
			}
		}
	}
#endif	
}

void IPNetwork::reset()
{
	time_t t = time(0);
	int d = int(t - last_reset_);
	if (d > 3) {
		last_reset_ = t;
		if (ssock_ >= 0) {
			(void)::close(ssock_);
			ssock_ = openssock(addr_, port_, ttl_);
		}
	}
}

int IPNetwork::openrsock(u_int32_t addr, u_short port,
			    const struct sockaddr_in& local)
{
	int fd;
	struct sockaddr_in sin;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		exit(1);
	}
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
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = port;
#ifdef IP_ADD_MEMBERSHIP
	if (IN_CLASSD(ntohl(addr))) {
		/*
		 * Try to bind the multicast address as the socket
		 * dest address.  On many systems this won't work
		 * so fall back to a destination of INADDR_ANY if
		 * the first bind fails.
		 */
		sin.sin_addr.s_addr = addr;
		if (::bind(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
			sin.sin_addr.s_addr = INADDR_ANY;
			if (::bind(fd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
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

		mr.imr_multiaddr.s_addr = addr;
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
		 * fails, another vic probably has the addresses bound so
		 * just exit.
		 */
		sin.sin_addr.s_addr = local.sin_addr.s_addr;
		if (::bind(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
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
		sin.sin_port = 0;
		sin.sin_addr.s_addr = addr;
		connect(fd, (struct sockaddr *)&sin, sizeof(sin));
#endif
	}
	/*
	 * XXX don't need this for the session socket.
	 */	
	int bufsize = 80 * 1024;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize,
			sizeof(bufsize)) < 0) {
		bufsize = 32 * 1024;
		if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize,
				sizeof(bufsize)) < 0)
			perror("SO_RCVBUF");
	}
	return (fd);
}

int IPNetwork::openssock(u_int32_t addr, u_short port, int ttl)
{
	int fd;
	struct sockaddr_in sin;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		exit(1);
	}
	nonblock(fd);

#ifdef WIN32
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	sin.sin_addr.s_addr = INADDR_ANY;
	if (::bind(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("bind");
		exit(1);
	}
#endif
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = port;
	sin.sin_addr.s_addr = addr;
	if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("connect");
		exit(1);
	}
	if (IN_CLASSD(ntohl(addr))) {
#ifdef IP_ADD_MEMBERSHIP
		char c;

		/* turn off loopback */
		c = 0;
		if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &c, 1) < 0) {
			/*
			 * If we cannot turn off loopback (Like on the
			 * Microsoft TCP/IP stack), then declare this
			 * option broken so that our packets can be
			 * filtered on the recv path.
			 */
			if (c == 0)
				noloopback_broken_ = 1;
		}
		/* set the multicast TTL */
#ifdef WIN32
		u_int t;
#else
		u_char t;
#endif
		t = (ttl > 255) ? 255 : (ttl < 0) ? 0 : ttl;
		if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL,
			       (char*)&t, sizeof(t)) < 0) {
			perror("IP_MULTICAST_TTL");
			exit(1);
		}
#else
		fprintf(stderr, "\
not compiled with support for IP multicast\n\
you must specify a unicast destination\n");
		exit(1);
#endif
	}
	/*
	 * XXX don't need this for the session socket.
	 */
	int bufsize = 80 * 1024;
	if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize,
		       sizeof(bufsize)) < 0) {
		bufsize = 48 * 1024;
		if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize,
			       sizeof(bufsize)) < 0)
			perror("SO_SNDBUF");
	}
	return (fd);
}
