/*
 * Copyright (c) 1995-1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/config.h,v 1.13 1997/12/19 22:20:12 bajaj Exp $ (LBL)
 */

#ifndef ns_config_h
#define ns_config_h


#if defined(sgi) || defined(__bsdi__) || defined(__FreeBSD__) || defined(linux)
#include <sys/types.h>
#else
/*XXX*/
#if defined(sun)
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
#endif
typedef signed char int8_t;
typedef unsigned char u_int8_t;
typedef short int16_t;
typedef unsigned short u_int16_t;
typedef unsigned int u_int32_t;
typedef int int32_t;
#endif

typedef int32_t nsaddr_t;
#define	NS_ALIGN	(8)	/* byte alignment for structs (eg packet.cc) */

#include <stdlib.h>
#if (defined(__hpux) || defined(_AIX)) && defined(__cplusplus)
#include <unistd.h>
#include <time.h>		/* For clock_t */
extern "C" {
#include <arpa/inet.h>
int strcasecmp(const char *, const char *);
clock_t clock(void);
int gethostid(void);
#if !defined(_AIX41) && !defined(sun)
void srandom(int);
#endif
long random(void);
time_t time(time_t *);
char *ctime(const time_t *);
}
#endif

#if defined(NEED_SUNOS_PROTOS) && defined(__cplusplus)
extern "C" {
struct timeval;
struct timezone;
int gettimeofday(struct timeval*, ...);
int ioctl(int fd, int request, ...);
int close(int);
int strcasecmp(const char*, const char*);
int srandom(int);	/* (int) for sunos, (unsigned) for solaris */
int random();
int socket(int, int, int);
int setsockopt(int s, int level, int optname, void* optval, int optlen);
struct sockaddr;
int connect(int s, sockaddr*, int);
int bind(int s, sockaddr*, int);
struct msghdr;
int send(int s, void*, int len, int flags);
int sendmsg(int, msghdr*, int);
int recv(int, void*, int len, int flags);
int recvfrom(int, void*, int len, int flags, sockaddr*, int);
int gethostid();
int getpid();
int gethostname(char*, int);
void abort();
}
#endif

#ifdef WIN32

#include <windows.h>
#include <winsock.h>
#include <time.h>		/* For clock_t */

#ifdef WIN32
#include <minmax.h>
#define NOMINMAX
#undef min
#undef max
#undef abs
#endif

#define MAXHOSTNAMELEN	256

#define _SYS_NMLN	9
struct utsname {
	char sysname[_SYS_NMLN];
	char nodename[_SYS_NMLN];
	char release[_SYS_NMLN];
	char version[_SYS_NMLN];
	char machine[_SYS_NMLN];
};

typedef char *caddr_t;

struct iovec {
	caddr_t iov_base;
	int	    iov_len;
};

#ifndef TIMEZONE_DEFINED_
#define TIMEZONE_DEFINED_
struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};
#endif

typedef int pid_t;
typedef int uid_t;
typedef int gid_t;
    
#if defined(__cplusplus)
extern "C" {
#endif

int uname(struct utsname *); 
int getopt(int, char * const *, const char *);
int strcasecmp(const char *, const char *);
#define srandom srand
#define random rand
int gettimeofday(struct timeval *p, struct timezone *z);
int gethostid(void);
int getuid(void);
int getgid(void);
int getpid(void);
int nice(int);
int sendmsg(int, struct msghdr*, int);
time_t time(time_t *);
        
#define bzero(dest,count) memset(dest,0,count)
#define bcopy(src,dest,size) memcpy(dest,src,size)
#if defined(__cplusplus)
}
#endif

#ifdef sgi
#include <math.h>
#endif

#define ECONNREFUSED	WSAECONNREFUSED
#define ENETUNREACH	WSAENETUNREACH
#define EHOSTUNREACH	WSAEHOSTUNREACH
#define EWOULDBLOCK	WSAEWOULDBLOCK

#define M_PI		3.14159265358979323846

#endif /* WIN32 */

//While changing these ensure that values are consistent with tcl/lib/ns-default.tcl
#define NODEMASK  0xffffff
#define NODESHIFT 8
#define PORTMASK  0xff

#endif

