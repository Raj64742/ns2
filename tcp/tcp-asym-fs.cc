/*
 * tcp-asym-fs: TCP mods for asymmetric networks (tcp-asym) with fast start
 * (tcp-fs).
 * 
 * Contributed by Venkat Padmanabhan (padmanab@cs.berkeley.edu), 
 * Daedalus Research Group, U.C.Berkeley
 */

#include "tcp-asym.h"
#include "tcp-fs.h"

/* TCP-FS with NewReno */
class NewRenoTcpAsymFsAgent : public NewRenoTcpAsymAgent, public NewRenoTcpFsAgent {
public:
	NewRenoTcpAsymFsAgent() : NewRenoTcpAsymAgent(), NewRenoTcpFsAgent() {count_bytes_acked_= 1;}

	/* helper functions */
	virtual void output_helper(Packet* pkt) {NewRenoTcpAsymAgent::output_helper(pkt); NewRenoTcpFsAgent::output_helper(pkt);}
	virtual void recv_helper(Packet* pkt) {NewRenoTcpAsymAgent::recv_helper(pkt); NewRenoTcpFsAgent::recv_helper(pkt);}
	virtual void send_helper(int maxburst) {NewRenoTcpFsAgent::send_helper(maxburst);}
	virtual void send_idle_helper() {NewRenoTcpFsAgent::send_idle_helper();}
	virtual void recv_newack_helper(Packet* pkt) {printf("count_bytes_acked_=%d\n", count_bytes_acked_); NewRenoTcpFsAgent::recv_newack_helper(pkt); NewRenoTcpAsymAgent::t_exact_srtt_ = NewRenoTcpFsAgent::t_exact_srtt_;}
	virtual void partialnewack_helper(Packet* pkt) {NewRenoTcpFsAgent::partialnewack_helper(pkt);}

	virtual void set_rtx_timer() {NewRenoTcpFsAgent::set_rtx_timer();}
	virtual void timeout_nonrtx(int tno) {NewRenoTcpFsAgent::timeout_nonrtx(tno);}
	virtual void timeout_nonrtx_helper(int tno) {NewRenoTcpFsAgent::timeout_nonrtx_helper(tno);}
};

static class NewRenoTcpAsymFsClass : public TclClass {
public:
	NewRenoTcpAsymFsClass() : TclClass("Agent/TCP/Newreno/Asym/FS") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new NewRenoTcpAsymFsAgent());
	}
} class_newrenotcpasymfs;	



