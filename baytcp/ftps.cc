/*
 * This agent is paired with one or more ftp clients.
 * It expects to be the target of a FullTcpAgent.
 */
#ifndef lint
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/baytcp/ftps.cc,v 1.2 2001/06/12 17:02:04 haldar Exp $ ()";
#endif

#include "tcp-full-bay.h"
#include "tclcl.h"
#include "random.h"
#include "trace.h"
#include "tcp.h"

class FtpSrvrAgent : public BayTcpAppAgent {
 public:
	FtpSrvrAgent();
	int command(int argc, const char*const* argv);
	void recv(Packet*, BayFullTcpAgent*);
protected:
	double now()  { return (Scheduler::instance().clock()); }
	int min_response_;	//number of bytes in min response file
	int max_response_;	//number of bytes in max response file
	int filesize_; 		//file size in Bytes
	static FILE* fp_;
};

#ifdef BAYTCP_DEBUG
FILE* FtpSrvrAgent::fp_ = fopen("ftpfilesize.tr", "w");
#else
FILE* FtpSrvrAgent::fp_ = NULL;
#endif

static class FtpSrvrClass : public TclClass {
public:
	FtpSrvrClass() : TclClass("Agent/BayTcpApp/FtpServer") {}
	TclObject* create(int, const char*const*) {
		return (new FtpSrvrAgent());
	}
} class_ftps;

FtpSrvrAgent::FtpSrvrAgent() : BayTcpAppAgent(PT_NTYPE)
{
//	bind("min_byte_response", &min_response_);
//	bind("max_byte_response", &max_response_);
//	bind_time("service_time", &service_time_);
	filesize_ = 0x10000000;
}

//should only be called when a get is received from a client
//need to make sure it goes to the right tcp connection

void FtpSrvrAgent::recv(Packet*, BayFullTcpAgent* tcp)
{
	int length = filesize_;
	//tells tcp-full with my mods to send FIN when empty
	tcp->advance(length, 1);
#ifdef BAYTCP_DEBUG
	fprintf(fp_, "server %g %d %d\n", now(), addr_, length);
#endif
}

/*
 * set length
 */
int FtpSrvrAgent::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "file_size") == 0) {
			filesize_ = atoi(argv[2]);
			return (TCL_OK); 
		}
	} 
	return (Agent::command(argc, argv));
}




