/* Ported from CMU/Monarch's code, nov'98 -Padma.
 * phy.h
 *
 * superclass for all network interfaces
 
 ==================================================================

 * Phy represents the hardware that actually controls the channel
 * access for the node. Phy transmits/receives pkts from the channel
 * to which it is connected. No pkts are buffered at this layer as
 * the decision to send has already been made and the packet is on
 * its way to the "Channel".
 *
 */

#ifndef ns_phy_h
#define ns_phy_h

#include <assert.h>
#include <bi-connector.h>
#include <list.h>

class Phy;
LIST_HEAD(if_head, Phy);

#include <new-channel.h>
#include <node.h>
#include <new-mac.h>

/*--------------------------------------------------------------
  Phy : Base class for all network interfaces used to control
  channel access
 ---------------------------------------------------------------*/

class Phy : public BiConnector {
 public:
	Phy();

	void recv(Packet* p, Handler* h);
	
	virtual void sendDown(Packet *p)=0;
	
	virtual int sendUp(Packet *p)=0;

	inline double  txtime(Packet *p) {
		return (hdr_cmn::access(p)->size() * 8.0) / bandwidth_; }
	inline double txtime(int bytes) {
		return (8.0 * bytes / bandwidth_); }
	virtual double  bittime() const { return 1/bandwidth_; }

	// list of all network interfaces, stored in two places
	Phy* nextchnl(void) const { return chnl_link_.le_next; }
	inline void insertchnl(struct if_head *head) {
		LIST_INSERT_HEAD(head, this, chnl_link_);
		//channel_ = chnl;
	}
	Phy* nextnode(void) const { return node_link_.le_next; }
	inline void insertnode(struct if_head* head) {
		LIST_INSERT_HEAD(head, this, node_link_);
		//node_ = node;
	}
	void setchnl (newChannel *chnl) { channel_ = chnl; }
	//virtual void setnode (Node *node) { node_ = node; }
	//virtual Node*	node(void) const { return node_; }
	
	
	virtual void    dump(void) const;

 protected:
	//void		drop(Packet *p);
	int		command(int argc, const char*const* argv);
	int             index_;

	//Node* node_;
  
	/*
   * A list of all "network interfaces" on a given channel.
   * Note: a node may have multiple interfaces, each of which
   * is on a different channel.
   */
	LIST_ENTRY(Phy) chnl_link_;

  /*
   * A list of all "network interfaces" for a given node.
   * Each interface is assoicated with exactly one node
   * and one channel.
   */
	LIST_ENTRY(Phy) node_link_;
	
/* ============================================================
     Physical Layer State
   ============================================================ */
	
	
	double bandwidth_;                   // bit rate
	newChannel         *channel_;    // the channel for output

};


#endif // ns_phy_h




