/*-
 * Copyright (c) 1993-1994 The Regents of the University of California.
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
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/net.cc,v 1.1 1997/05/14 02:47:28 mccanne Exp $ (LBL)";

#include <stdlib.h>
#include <math.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#ifdef WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#include <sys/uio.h>
#endif
#include "net.h"

/*
 * Linux does not have sendmsg
 */
#if defined(__linux__) || defined(WIN32)
#define MAXPACKETSIZE (1500-28)

static int
sendmsg(int s, struct msghdr* mh, int flags)
{
	u_char wrkbuf[MAXPACKETSIZE];
	int len = mh->msg_iovlen;
	struct iovec* iov = mh->msg_iov;
	u_char* cp;
	u_char* ep;

	for (cp = wrkbuf, ep = wrkbuf + MAXPACKETSIZE; --len >= 0; ++iov) {
		int plen = iov->iov_len;
		if (cp + plen >= ep) {
			errno = E2BIG;
			return (-1);
		}
		memcpy(cp, iov->iov_base, plen);
		cp += plen;
	}
	return (send(s, (char*)wrkbuf, cp - wrkbuf, flags));
}
#endif

Network::Network() :
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

Network::~Network()
{
}

int Network::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		Tcl& tcl = Tcl::instance();
		char* cp = tcl.buffer();
		if (strcmp(argv[1], "addr") == 0 || 
		    strcmp(argv[1], "interface") == 0 ||
		    strcmp(argv[1], "port") == 0 ||
		    strcmp(argv[1], "ttl") == 0 ||
		    strcmp(argv[1], "ismulticast") == 0)
			strcpy(cp, "0");
		else if (strcmp(argv[1], "flush") == 0) {
			u_int32_t from;		    
			while (dorecv(wrkbuf_, wrkbuflen_, from, rsock_) > 0)
				;
		} else
			return (TclObject::command(argc, argv));
		tcl.result(cp);
		return (TCL_OK);
	} else if (argc == 3) {
	}
	return (TclObject::command(argc, argv));
}

void Network::nonblock(int fd)
{       
#ifdef WIN32
	u_long flag = 1;
	if (ioctlsocket(fd, FIONBIO, &flag) == -1) {
		fprintf(stderr, "ioctlsocket: FIONBIO: %lu\n", GetLastError());
		exit(1);
	}
#else
        int flags = fcntl(fd, F_GETFL, 0);
#if defined(hpux) || defined(__hpux)
        flags |= O_NONBLOCK;
#else
        flags |= O_NONBLOCK|O_NDELAY;
#endif
        if (fcntl(fd, F_SETFL, flags) == -1) {
                perror("fcntl: F_SETFL");
                exit(1);
        }
#endif
}

u_char* Network::wrkbuf_;
int Network::wrkbuflen_;

void Network::expand_wrkbuf(int len)
{
	if (wrkbuflen_ == 0)
		wrkbuf_ = (u_char*)malloc(len);
	else
		wrkbuf_ = (u_char*)realloc((u_char*)wrkbuf_, len);
	wrkbuflen_ = len;
}

void Network::dosend(u_char* buf, int len, int fd)
{
	int cc = ::send(fd, (char*)buf, len, 0);
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

void Network::send(u_char* buf, int len)
{
	dosend(buf, len, ssock_);
}

#ifdef notdef
void Network::send(const pktbuf* pb)
{
	/*XXX*/
	send(pb->dp, pb->len);
}
#endif

int Network::dorecv(u_char* buf, int len, u_int32_t& from, int fd)
{
	sockaddr_in sfrom;
	int fromlen = sizeof(sfrom);
	int cc = ::recvfrom(fd, (char*)buf, len, 0,
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

int Network::recv(u_char* buf, int len, u_int32_t& from)
{
	return (dorecv(buf, len, from, rsock_));
}

void Network::reset()
{
}
