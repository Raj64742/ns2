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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/emulate/net-pcap.cc,v 1.1 1998/01/07 21:59:16 kfall Exp $ (LBL)";
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

#define	PNET_SRCTYPE_CHANNEL	0
#define	PNETN_SRCTYPE_LIVE	1

class PcapNetwork : public Network {

public:
	PcapNetwork();
	virtual int command(int argc, const char*const* argv);
	virtual void reset();
protected:
	virtual int open();
	virtual int open(char*);
	virtual void bindvars();

	void reset();
	int close();

	char errbuf_[PCAP_ERRBUF_SIZE];
	char *devname_;
	int state_;
	int optimize_;
	pcap_t pcap_;
	struct bpf_program bpfpgm_;
};

class PcapLiveNetwork : public PcapNetwork {
public:
	int open();
	int open(char*);
protected:
	int snaplen_;
	int promisc_;
	int livetimeout_;
}

static class PcapLiveNetWork : public TclClass {
    public:
	PcapLiveNetWork() : TclClass("Network/Pcap/Live") {}
	TclObject* create(int, const char*const*) {
		return (new PcapNetwork);
	}
} net_pcaplive;

static class PcapChanNetWork : public TclClass {
    public:
	PcapChanNetWork() : TclClass("Network/Pcap/Channel") {}
	TclObject* create(int, const char*const*) {
		return (new PcapNetwork);
	}
} net_pcapchan;

void
PcapNetwork::PcapNetwork()
{
	reset();
	bindvars();
}

void
PcapNetwork::bindvars()
{
	bind("optimize_", &optimize_);
}

void
PcapNetwork::reset()
{
	state_ = PSTATE_INACTIVE;
	*errbuf_ = '\0';
}

int
PcapNetwork::open()
{
	return (open(autodevname()));
}

int
PcapNetwork::open(char *dname)
{
	// to_ms parameter zero means don't set timeout
	if (srctype_ == PNETN_SRCTYPE_LIVE) {
		pcap_ = pcap_open_live(devname_,
			snaplen_, promisc_, livetimeout_, errbuf_);
	} else if (srctype_ == PNET_SRCTYPE_CHANNEL) {
		pcap_ = pcap_open_offline(devname_, errbuf_);
	}

	if (pcap_ == NULL) {
		fprintf(stderr,
		  "pcap object (%s) couldn't open packet source %s: %s\n",
			name(), devname_, errbuf_);
		return -1;
	}
	devname_ = dname;
	return 0;
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
PcapNetwork::bpf_compile(char *pgm)
{
	bpf_u_int32 mask = bpf_u_int32(0x0);
	return(pcap_compile(pcap_, &bpfpgm_, pgm, optimize_, mask));
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
		if (strcmp(argv[1], "open") == 0) {
			if (strcmp(argv[2], "live") == 0) {
				srctype_ = PNET_SRCTYPE_LIVE;
			} else {
				tcl.resultf(
				   "pcap obj (%s): invalid source %s",
				   name(), argv[2]);
				return (TCL_ERROR);
			}
			if (open() < 0)
				return (TLC_ERROR);
			tcl.result("1");
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "open") == 0) {
			if (strcmp(argv[2], "chan") == 0) {
				srctype_ = PNET_SRCTYPE_CHAN;
			else if (strcmp(argv[2], "live") == 0) {
				srctype_ = PNET_SRCTYPE_LIVE;
			} else {
				tcl.resultf(
				   "pcap obj (%s): unknown src type %s\n"
				   name(), argv[2]);
				return (TCL_ERROR);
			}
			if (open(argv[3]) < 0) {
				return (TCL_ERROR);
			}
			tcl.result("1");
			return (TCL_OK);
		}
	return (Network::command(argc, argv));
}

int PcapNetwork::open(u_int32_t addr, int port, int ttl)
{
	return (0);
}

int PcapNetwork::open(int port)
{
}

int PcapNetwork::close()
{
	return (0);
}


char *
PcapLiveNetwork::autodevname()
{
	char *dname;
	if ((dname = pcap_lookupdev(errbuf_)) == NULL) {
		fprintf(stderr, "warning: PcapNet(%s) : %s\n",
			name(), errbuf_);
		return;
	}
	return dname;
}

void
PcapLiveNetwork::bindvars()
{
	bind("snaplen_", &snaplen_);
	bind("promisc_", &promisc_);
	bind("livetimeout_", &livetimeout_);
	PcapNetwork::bindvars();
}
