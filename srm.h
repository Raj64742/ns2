/* This is the port of the initial SRM implementation in ns-1 by
   Kannan Varadahan <kannan@isi.edu>. Most of the code is in C++. Since
   then, there has been another implementation. The new implemenatation
   has better c++/tcl separation. Check later releases for new
   implementation. This code has been checked in for archival reasons.
   -Puneet Sharma <puneet@isi.edu>
*/

#ifndef ns_srm_h
#define ns_srm_h

#include "config.h"
#include "packet.h"
#include "ip.h"
#include <assert.h>
#include <Tcl.h>

/* Traffic Generator to attach to the SRM sender */
#include "trafgen.h"
class TrafficGenerator;


/* static char*srmNames[]= {"none","data","session","request","repair"}; */

class srmEvent;


/* Macro for initializing a new message block, 
   used for maintaining state about various messages */

#define GET_NEW_MSGINFO_BLOCK do{ \
msg_info*mnew= new msg_info; \
mnew->sender_= this; \
new->id_= msgid; \
mnew->status_= SRM_UNKNOWN; \
mnew->backoff_const_= -1; \
mnew->ev_= 0; \
mnew->next_= *mp; \
*mp= mnew; \
}while(0) ;

#define NOW Scheduler::instance().clock()

#define RESET_BACKOFF(m) (m) ->backoff_const_= 1;
#define SRM_NONE 0
#define SRM_DATA 1
#define SRM_SESS 2
#define SRM_RQST 3
#define SRM_REPR 4
#define SRM_UNKNOWN 0
#define SRM_DATA_RECEIVED 1
#define SRM_RQST_PENDING 2
#define SRM_REPR_PENDING 3
#define SRM_REPR_WAIT 4
#define SRM_SESSION_TIMER 0
#define SRM_SESSION_DELAY 1
#define C1 2.0
#define C2 2.0
#define D1 1.0
#define D2 1.0
#define MAX_PKTS 10
#define MAX_SRM_DATA_SIZE 1000


#define CANCEL_EVENT(ev) do{ \
if(ev)  \
Scheduler::instance() .cancel(ev) ; \
}while(0) 

/* Extract state info from session message */
#define GET_SESSION_INFO1  \
sender= msg->srcid(); \
msgid= msg->id(); \
sessctr= msg->session_ctr(); \
rtime= msg->rtime(); \
stime= msg->stime(); \

/* Extract state info from session message */
#define GET_SESSION_INFO  \
sender= s_info->srcid_; \
msgid= s_info->id_; \
sessctr= s_info->session_ctr_; \
rtime= s_info->rtime_; \
stime= s_info->stime_; \


struct hdr_srm {
	char var_data_[MAX_SRM_DATA_SIZE];
	u_int32_t srcid_;
	double ts_;		/* time packet generated (at source) */
	int type_;
	int session_ctr_;
	int stime_;
	int rtime_;
	int id_;
	int sender_;          /* Sender of the related data unit */
	int num_entries_;     /* Number of entries for various sources */

        /* per-field member functions */
	char* var_data() {
	  return (var_data_);
	}
	u_int32_t& srcid() {
	  return (srcid_);
	}
	int& type() {
	  return (type_);
	}
	int& session_ctr() {
	  return (session_ctr_);
	}
	int& stime() {
	  return (stime_);
	}
	int& rtime() {
	  return (rtime_);
	}
	int& id() {
	  return (id_);
	}
	int& sender() {
	  return (sender_);        
	}
	int& num_entries() {
	  return (num_entries_);   
	}
	
};




/* Structure for session state information */
struct session_info{
  u_int32_t srcid_;
  int session_ctr_;
  int stime_;
  int rtime_;
  int id_;
};

class SRMSource;

/* Structure for storing message state */
struct msg_info{
  msg_info *next_;
  SRMSource *sender_;   /* Original sender of the packet */
  int id_;              /* msg id, seq_no */
  int status_;          /* Message status */
  int backoff_const_;   
  double ignore_backoff_;
  int r_src_;   /* Request/repair source */
  Event*ev_;
};


class SRMSource : public TclObject {
public:
	SRMSource* next_;
	int src_;                  /* Src id */
	int received_;             /* Received packet id */
	int sent_;                 /* What has been sent */
	int sess_ctr_sent_;        /* Last known session message counter */
	int recvt_;                /* Recv time for the last session message */
	int sendt_;                /* Time last session message was sent */
	double distance_;
	SRMSource(unsigned int srcid);
	struct msg_info *messages_, *cache_;
	msg_info *get_msg_info(int msgid);
	int status(int msgid);
	void status(int msgid,int status);
	void set_event(int msgid,Event*ev);
	void cancel_event(int msgid);
	int backoff(int msgid);
	void reset_backoff(int msgid);
	void incr_backoff(int msgid);
	double backoff_timer(int msgid);
	void backoff_timer(int msgid,double t);
	void set_rsrc(int msgid, int r_src);
	int rsrc(int msgid);
	void update_vars();
};

class SRMAgent : public Agent {
public:
        double C_[3];       /* Repair/Request timer constants */
	double D_[3];       /* Repair/Request timer constants */
	int max_packets_;   
	int session_ctr_;
	int off_srm_;
	SRMSource* localsrc_;
	TrafficGenerator *trafgen_;
	int running_;
	int sender_running_;
	SRMAgent();
	void recv_data(int sender, int msgid);
	SRMSource *get_sender_state(int sender);
	void start();
	void start_sender();
	void request(int sender,int msgid);
	void recv_rqst(int sender, int msgid, int rqst_src);
	void repair(int sender,int msgid, int rqst_src);
	void recv_repr(int sender, int msgid);
	void send_session();
	void recv_sess(hdr_srm*msg);
	inline srmEvent*schedule_send_event(double time,int type,int sndr,int id);
	inline void send_packet(int type,int sndr,int id, int size);
	inline void transmit(Packet*p,int type,int sndr,int id);
	void send_data();
	void send_rqst(int sender,int msgid);
	void send_repr(int sender,int msgid);
	void timeout(int tno);
	~SRMAgent(){};
	int command(int argc,const char*const*argv);
	void recv(Packet*p, Handler*);
	void print_groupstate();
protected:
	SRMSource* allsrcs_;
	int seq_no_;
};


#endif

