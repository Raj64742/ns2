/*-
 * Copyright (c) 1998 The Regents of the University of California.
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/emulate/net-pcap.cc,v 1.9 1998/02/21 03:03:09 kfall Exp $ (LBL)";
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

#ifdef __cplusplus
extern "C" {
#include <pcap.h>
}
#else
#include <pcap.h>
#endif

#include "config.h"
#include "net.h"
#include "tclcl.h"

/*
 * observations about pcap library
 *	device name is in the ifreq struct sense
 *	pcap_lookupdev returns a ptr to static data
 *	q: does lookupdev only return devs in the AF_INET addr family?
 *	why does pcap_compile require a netmask? seems odd
 *	would like some way to tell it what buffer to use
 *	arriving packets have the link layer hdr at the beginning
 *	not convenient to open bpf read/write
 *	no real way to know what file (/dev/bpf?) it is using
 *		would be nice if pcap_lookdev helped out more by
 *		returning ifnet or ifreq or whatever structure
 *	pcap_lookupnet makes calls to get our addr, but
 *		then tosses it anyhow
 *	interface type codes could be via rfc1573
 *		see freebsd net/if_types.h
 *	want a way to set immed mode
 *	
 */

#define	PNET_PSTATE_INACTIVE	0
#define	PNET_PSTATE_ACTIVE	1

//
// PcapNetwork: a "network" (source or possibly sink of packets)
//	this is a base class only-- the derived classes are:
//	PcapLiveNetwork [a live net; currently bpf + ethernet]
//	PcapFileNetwork [packets from a tcpdump-style trace file]
//

class PcapNetwork : public Network {

public:
	PcapNetwork() : local_netmask_(0), pfd_(-1) { }
	int rchannel() { return(pfd_); }
	int schannel() { return(pfd_); }
	virtual int command(int argc, const char*const* argv);

	virtual int open(const char*) = 0;
	virtual int skiphdr() = 0;
	int recv(u_char *buf, int len, sockaddr&);		// get from net
	int send(u_char *buf, int len);			// write to net
	void close();
	void reset();

	int filter(const char*);	// compile + install a filter
	int stat_pkts();
	int stat_pdrops();

protected:
	virtual void bindvars();

	char errbuf_[PCAP_ERRBUF_SIZE];		// place to put err msgs
	char srcname_[PATH_MAX];		// device or file name
	int pfd_;				// pcap fd
	int state_;				// PNET_PSTATE_xxx (above)
	int optimize_;				// bpf optimizer enable
	pcap_t* pcap_;				// reference to pcap state
	struct bpf_program bpfpgm_;		// generated program
	struct pcap_pkthdr pkthdr_;		// pkt hdr to fill in
	struct pcap_stat pcs_;			// status

	unsigned int local_netmask_;	// seems shouldn't be necessary :(
};

//
// PcapLiveNetwork: a live network tap
//

struct NetworkAddress {
        u_int   len_;
        u_char  addr_[16];	// enough for IPv6 ip addr
};

class PcapLiveNetwork : public PcapNetwork {
public:
	PcapLiveNetwork() : local_net_(0), dlink_type_(-1) {
		bindvars(); reset();
	}
	NetworkAddress& laddr() { return (linkaddr_); }
	NetworkAddress& naddr() { return (netaddr_); }
protected:
	int open();
	int open(const char*);
	int command(int argc, const char*const* argv);
	int skiphdr();
	const char*	autodevname();
	void	bindvars();

	int snaplen_;		// # of bytes to grab
	int promisc_;		// put intf into promisc mode?
	double timeout_;
	NetworkAddress	linkaddr_;	// link-layer address
	NetworkAddress	netaddr_;	// network-layer (IP) address

	unsigned int local_net_;
	int dlink_type_;		// data link type (see pcap)

private:

	// XXX somewhat specific to bpf-- this stuff is  a hack until pcap
	// can be fixed to allow for opening the bpf r/w
	pcap_t * pcap_open_live(char *, int slen, int prom, int, char *, int);
	int bpf_open(pcap_t *p, char *errbuf, int how);
};

class PcapFileNetwork : public PcapNetwork {
public:
	int open(const char*);
	int skiphdr() { return 0; }	// XXX check me
protected:
	int command(int argc, const char*const* argv);
};

static class PcapLiveNetworkClass : public TclClass {
public:
	PcapLiveNetworkClass() : TclClass("Network/Pcap/Live") {}
	TclObject* create(int, const char*const*) {
		return (new PcapLiveNetwork);
	}
} net_pcaplive;

static class PcapFileNetworkClass : public TclClass {
public:
	PcapFileNetworkClass() : TclClass("Network/Pcap/File") {}
	TclObject* create(int, const char*const*) {
		return (new PcapFileNetwork);
	}
} net_pcapfile;

//
// defs for base PcapNetwork class
//

void
PcapNetwork::bindvars()
{
	bind_bool("optimize_", &optimize_);
}

void
PcapNetwork::reset()
{
	state_ = PNET_PSTATE_INACTIVE;
	pfd_ = -1;
	pcap_ = NULL;
	*errbuf_ = '\0';
	*srcname_ = '\0';
}


void
PcapNetwork::close()
{
	if (state_ == PNET_PSTATE_ACTIVE && pcap_)
		pcap_close(pcap_);
	reset();
}

/* compile up a bpf program */
/* XXXwe aren't using 'bcast', so don't care about mask... sigh */

int
PcapNetwork::filter(const char *pgm)
{
	if (pcap_compile(pcap_, &bpfpgm_, (char *)pgm,
	    optimize_, local_netmask_) < 0) {
		fprintf(stderr, "pcapnet obj(%s): couldn't compile filter pgm",
			name());
		return -1;
	}
	if (pcap_setfilter(pcap_, &bpfpgm_) < 0) {
		fprintf(stderr, "pcapnet obj(%s): couldn't set filter pgm",
			name());
		return -1;
	}
	return(bpfpgm_.bf_len);
}

/* return number of pkts received */
int
PcapNetwork::stat_pkts()
{
	if (pcap_stats(pcap_, &pcs_) < 0)
		return (-1);

	return (pcs_.ps_recv);
}

/* return number of pkts dropped */
int
PcapNetwork::stat_pdrops()
{
	if (pcap_stats(pcap_, &pcs_) < 0)
		return (-1);

	return (pcs_.ps_drop);
}

#ifndef MIN
#define MIN(x, y) ((x)<(y) ? (x) : (y))
#endif

#include "ether.h"
/* recv is what others call to grab a packet from the pfilter */
int
PcapNetwork::recv(u_char *buf, int len, sockaddr& fromaddr)
{
	if (state_ != PNET_PSTATE_ACTIVE) {
		fprintf(stderr, "warning: net/pcap obj(%s) read-- not active\n",
			name());
		return -1;
	}

	const u_char *pkt;
	pkt = pcap_next(pcap_, &pkthdr_);
	int n = MIN(pkthdr_.caplen, len);
	// link layer header will be placed at the beginning
	int s = skiphdr();
	memcpy(buf, pkt + s, n - s);

Ethernet::ether_print(pkt);
	return n - s;
}

/* send a packet out through the packet filter */
int
PcapNetwork::send(u_char *buf, int len)
{
	int n;

printf("PcapNetwork(%s): writing %d bytes to bpf fd %d\n",
name(), n, pfd_);
Ethernet::ether_print(buf);

	if ((n = write(pfd_, buf, len)) < 0)
		perror("write to pcap fd");

	return n;
}

int PcapNetwork::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "close") == 0) {
			close();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "srcname") == 0) {
			tcl.result(srcname_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "pkts") == 0) {
			tcl.resultf("%d", stat_pkts());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "pdrops") == 0) {
			tcl.resultf("%d", stat_pdrops());
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "filter") == 0) {
			if (state_ != PNET_PSTATE_ACTIVE) {
				fprintf(stderr, "net/pcap obj(%s): can't install filter prior to opening data source\n",
					name());
				return (TCL_ERROR);
			}
			int plen;
			if ((plen = filter(argv[2])) < 0) {
				fprintf(stderr, "problem compiling/installing filter program\n");
				return (TCL_ERROR);
			}
			tcl.resultf("%d", plen);
			return (TCL_OK);
		}
	}
	return (Network::command(argc, argv));
}

//
// defs for PcapLiveNetwork
//

#include <fcntl.h>

#include <net/if.h>
int
PcapLiveNetwork::open(const char *devname)
{
	close();
	pcap_ = pcap_open_live((char*) devname, snaplen_, promisc_,
		int(timeout_ * 1000.), errbuf_, O_RDWR);

	if (pcap_ == NULL) {
		fprintf(stderr,
		  "pcap/live object (%s) couldn't open packet source %s: %s\n",
			name(), devname, errbuf_);
		return -1;
	}
	dlink_type_ = pcap_datalink(pcap_);
	pfd_ = pcap_fileno(pcap_);
	strncpy(srcname_, devname, sizeof(srcname_)-1);
	{
		// use SIOCGIFADDR hook in bpf to get link addr
		struct ifreq ifr;
		struct sockaddr *sa = &ifr.ifr_addr;
		if (ioctl(pfd_, SIOCGIFADDR, &ifr) < 0) {
			fprintf(stderr,
			  "pcap/live (%s) SIOCGIFADDR on bpf fd %d\n",
			  name(), pfd_);
		}
		if (dlink_type_ != DLT_EN10MB) {
			fprintf(stderr,
				"sorry, only ethernet supported\n");
			return -1;
		}
		linkaddr_.len_ = ETHER_ADDR_LEN;	// for now
		memcpy(linkaddr_.addr_, sa->sa_data, linkaddr_.len_);
	}

	state_ = PNET_PSTATE_ACTIVE;

	if (pcap_lookupnet(srcname_, &local_net_, &local_netmask_, errbuf_) < 0) {
		fprintf(stderr,
		  "warning: pcap/live (%s) couldn't get local IP network info: %s\n",
		  name(), errbuf_) ;
	}

	{
		int immed = 1;
		if (ioctl(pfd_, BIOCIMMEDIATE, &immed) < 0) {
			fprintf(stderr,
				"warning: pcap/live (%s) couldn't set immed\n",
				name());
		}
	}
	return 0;
}

/*
 * how many bytes of link-hdr to skip before net-layer hdr
 */

int
PcapLiveNetwork::skiphdr()
{
	switch (dlink_type_) {
	case DLT_NULL:
		return 0;

	case DLT_EN10MB:
		return ETHER_HDR_LEN;

	default:
		fprintf(stderr,
		    "Network/Pcap/Live(%s): unknown link type: %d\n",
			dlink_type_);
	}
	return -1;
}

const char *
PcapLiveNetwork::autodevname()
{
	const char *dname;
	if ((dname = pcap_lookupdev(errbuf_)) == NULL) {
		fprintf(stderr, "warning: PcapNet/Live(%s) : %s\n",
			name(), errbuf_);
		return (NULL);
	}
	return (dname);	// ptr to static data in pcap library
}

void
PcapLiveNetwork::bindvars()
{
	bind("snaplen_", &snaplen_);
	bind_bool("promisc_", &promisc_);
	bind_time("timeout_", &timeout_);
	PcapNetwork::bindvars();
}

int
PcapLiveNetwork::open()
{
	return (open(autodevname()));
}

int PcapLiveNetwork::command(int argc, const char*const* argv)
{

	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		// $obj open
		if (strcmp(argv[1], "open") == 0) {
			if (open() < 0)
				return (TCL_ERROR);
			tcl.result(srcname_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "linkaddr") == 0) {
			/// XXX: only for ethernet now
			tcl.result(Ethernet::etheraddr_string(linkaddr_.addr_));
			return (TCL_OK);
		}
	} else if (argc == 3) {
		// $obj open "devicename"
		if (strcmp(argv[1], "open") == 0) {
			if (open(argv[2]) < 0)
				return (TCL_ERROR);
			tcl.result(srcname_);
			return (TCL_OK);
		}
	}
	return (PcapNetwork::command(argc, argv));
}

//
// defs for PcapFileNetwork
// use a file instead of a live network
//

int
PcapFileNetwork::open(const char *filename)
{

	close();
	pcap_ = pcap_open_offline((char*) filename, errbuf_);
	if (pcap_ == NULL) {
		fprintf(stderr,
		  "pcap/file object (%s) couldn't open packet source %s: %s\n",
			name(), filename, errbuf_);
		return -1;
	}
	pfd_ = pcap_fileno(pcap_);
	strncpy(srcname_, filename, sizeof(srcname_)-1);
	state_ = PNET_PSTATE_ACTIVE;
	return 0;
}

int PcapFileNetwork::command(int argc, const char*const* argv)
{

	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		// $obj open filename
		if (strcmp(argv[1], "open") == 0) {
			if (open(argv[2]) < 0)
				return (TCL_ERROR);
			tcl.result("1");
			return (TCL_OK);
		}
	}
	return (PcapNetwork::command(argc, argv));
}

//
// XXX: the following routines are unfortunately necessary, 
//	because libpcap has no obvious was of making the bpf fd
//	be read-write :(.  The implication here is nasty:
//		our own version of bpf_open and pcap_open_live
//		and the later routine requires the struct pcap internal state

/*   
 * Savefile
 */  
struct pcap_sf {
        FILE *rfile; 
        int swapped;
        int version_major;
        int version_minor;
        u_char *base;
};   
     
struct pcap_md {
        struct pcap_stat stat;
        /*XXX*/ 
        int use_bpf;
        u_long  TotPkts;        /* can't oflow for 79 hrs on ether */
        u_long  TotAccepted;    /* count accepted by filter */
        u_long  TotDrops;       /* count of dropped packets */
        long    TotMissed;      /* missed by i/f during this run */
        long    OrigMissed;     /* missed by i/f before this run */
#ifdef linux
        int pad;
        int skip;
        char *device;
#endif
};   


struct pcap {
        int fd;
        int snapshot;
        int linktype;
        int tzoff;              /* timezone offset */
        int offset;             /* offset for proper alignment */

        struct pcap_sf sf;
        struct pcap_md md;

        /*
         * Read buffer.
         */
        int bufsize;
        u_char *buffer;
        u_char *bp;
        int cc;

        /*
         * Place holder for pcap_next().
         */
        u_char *pkt;

        
        /*
         * Placeholder for filter code if bpf not in kernel.
         */
        struct bpf_program fcode;

        char errbuf[PCAP_ERRBUF_SIZE];
};


#include <net/if.h>

int
PcapLiveNetwork::bpf_open(pcap_t *p, char *errbuf, int how)
{
        int fd;
        int n = 0;
        char device[sizeof "/dev/bpf000"];

        /*
         * Go through all the minors and find one that isn't in use.
         */
        do {
                (void)sprintf(device, "/dev/bpf%d", n++);
                fd = ::open(device, how, 0);
        } while (fd < 0 && n < 1000 && errno == EBUSY);

        /*
         * XXX better message for all minors used
         */
        if (fd < 0)
                sprintf(errbuf, "%s: %s", device, pcap_strerror(errno));

        return (fd);
}

pcap_t *
PcapLiveNetwork::pcap_open_live(char *device, int snaplen, int promisc, int to_ms, char *ebuf, int how)
{
        int fd;
        struct ifreq ifr;
        struct bpf_version bv;
        u_int v;
        pcap_t *p;

        p = (pcap_t *)malloc(sizeof(*p));
        if (p == NULL) {
                sprintf(ebuf, "malloc: %s", pcap_strerror(errno));
                return (NULL);
        }
        bzero(p, sizeof(*p));
        fd = bpf_open(p, ebuf, how);
        if (fd < 0)
                goto bad;

        p->fd = fd;
        p->snapshot = snaplen;

        if (ioctl(fd, BIOCVERSION, (caddr_t)&bv) < 0) {
                sprintf(ebuf, "BIOCVERSION: %s", pcap_strerror(errno));
                goto bad;
        }
        if (bv.bv_major != BPF_MAJOR_VERSION ||
            bv.bv_minor < BPF_MINOR_VERSION) {
                sprintf(ebuf, "kernel bpf filter out of date");
                goto bad;
        }
        (void)strncpy(ifr.ifr_name, device, sizeof(ifr.ifr_name));
        if (ioctl(fd, BIOCSETIF, (caddr_t)&ifr) < 0) {
                sprintf(ebuf, "%s: %s", device, pcap_strerror(errno));
                goto bad;
        }
        /* Get the data link layer type. */
        if (ioctl(fd, BIOCGDLT, (caddr_t)&v) < 0) {
                sprintf(ebuf, "BIOCGDLT: %s", pcap_strerror(errno));
                goto bad;
        }
#if _BSDI_VERSION - 0 >= 199510
        /* The SLIP and PPP link layer header changed in BSD/OS 2.1 */
        switch (v) {

        case DLT_SLIP:
                v = DLT_SLIP_BSDOS;
                break;
        case DLT_PPP:
                v = DLT_PPP_BSDOS;
                break;
        }
#endif  
        p->linktype = v;

        /* set timeout */
        if (to_ms != 0) {
                struct timeval to;
                to.tv_sec = to_ms / 1000;
                to.tv_usec = (to_ms * 1000) % 1000000;
                if (ioctl(p->fd, BIOCSRTIMEOUT, (caddr_t)&to) < 0) {
                        sprintf(ebuf, "BIOCSRTIMEOUT: %s",
                                pcap_strerror(errno));
                        goto bad;
                }
        }
        if (promisc)
                /* set promiscuous mode, okay if it fails */
                (void)ioctl(p->fd, BIOCPROMISC, NULL); 

        if (ioctl(fd, BIOCGBLEN, (caddr_t)&v) < 0) {
                sprintf(ebuf, "BIOCGBLEN: %s", pcap_strerror(errno));
                goto bad;
        }   
        p->bufsize = v;
        p->buffer = (u_char *)malloc(p->bufsize);
        if (p->buffer == NULL) {
                sprintf(ebuf, "malloc: %s", pcap_strerror(errno));
                goto bad;
        }       

        return (p);
 bad:   
        ::close(fd);
        free(p);
        return (NULL);
}
