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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/emulate/net-pcap.cc,v 1.3 1998/01/07 23:40:55 kfall Exp $ (LBL)";
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
 */

#define	PNET_PSTATE_INACTIVE	0
#define	PNET_PSTATE_ACTIVE	1

class PcapNetwork : public Network {

public:
	PcapNetwork() : local_netmask_(0) { }
	virtual int command(int argc, const char*const* argv);
protected:
	virtual int open(const char*) = 0;
	virtual void bindvars();

	int filter(const char*);	// compile + install a filter

	void reset();
	void close();

	char errbuf_[PCAP_ERRBUF_SIZE];
	char srcname_[PATH_MAX];		// device of file name
	int state_;
	int optimize_;				// bpf optimizer enable
	pcap_t* pcap_;				// reference to pcap state
	struct bpf_program bpfpgm_;		// generated program

	unsigned int local_netmask_;	// seems shouldn't be necessary :(
};

class PcapLiveNetwork : public PcapNetwork {
public:
	PcapLiveNetwork() : local_net_(0) {
		bindvars(); reset();
	}
protected:
	int open();
	int open(const char*);
	int command(int argc, const char*const* argv);
	const char*	autodevname();
	void	bindvars();

	int snaplen_;
	int promisc_;
	int livetimeout_;

	unsigned int local_net_;
};

class PcapFileNetwork : public PcapNetwork {
public:
	int open(const char*);
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
	bind("optimize_", &optimize_);
}

void
PcapNetwork::reset()
{
	state_ = PNET_PSTATE_INACTIVE;
	*errbuf_ = '\0';
	*srcname_ = '\0';
}


void
PcapNetwork::close()
{
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
	return 0;
}

int PcapNetwork::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "close") == 0) {
			close();
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "filter") == 0) {
			if (state_ != PNET_PSTATE_ACTIVE) {
				tcl.resultf("net/pcap obj(%s): can't install filter prior to opening data source\n",
					name());
				return (TCL_ERROR);
			}
			if (filter(argv[2]) < 0) {
				tcl.resultf("problem compiling/installing filter program");
				return (TCL_ERROR);
			}
			tcl.result("1");
			return (TCL_OK);
		}
	}
	return (Network::command(argc, argv));
}

//
// defs for PcapLiveNetwork
//

int
PcapLiveNetwork::open(const char *devname)
{
	pcap_ = pcap_open_live(srcname_, snaplen_, promisc_,
		livetimeout_, errbuf_);

	if (pcap_ == NULL) {
		fprintf(stderr,
		  "pcap/live object (%s) couldn't open packet source %s: %s\n",
			name(), srcname_, errbuf_);
		return -1;
	}
	strncpy(srcname_, devname, sizeof(srcname_)-1);
	state_ = PNET_PSTATE_ACTIVE;

	if (pcap_lookupnet(srcname_, &local_net_, &local_netmask_, errbuf_) < 0) {
		fprintf(stderr,
		  "warning: pcap/live (%s) couldn't get local IP network info: %s\n",
		  name(), errbuf_) ;
	}

	return 0;
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
	bind("promisc_", &promisc_);
	bind("livetimeout_", &livetimeout_);
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
			return (TCL_OK);
		}
	} else if (argc == 3) {
		// $obj open "devicename"
		if (strcmp(argv[1], "open") == 0) {
			if (open(argv[2]) < 0)
				return (TCL_ERROR);
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
	pcap_ = pcap_open_offline((char*) filename, errbuf_);
	if (pcap_ == NULL) {
		fprintf(stderr,
		  "pcap/file object (%s) couldn't open packet source %s: %s\n",
			name(), srcname_, errbuf_);
		return -1;
	}
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
			return (TCL_OK);
		}
	}
	return (PcapNetwork::command(argc, argv));
}
