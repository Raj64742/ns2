// Address parameters. Singleton class
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/addr-params.h,v 1.1 2000/09/14 18:22:59 haoboy Exp $

#ifndef ns_addr_params_h
#define ns_addr_params_h

#include <tclcl.h>

class AddrParamsClass : public TclClass{
public:
	// XXX Default hlevel_ to 1.
	AddrParamsClass() : hlevel_(1), NodeMask_(NULL), NodeShift_(NULL),
			    TclClass("AddrParams") {
		instance_ = this;
	}
	virtual TclObject* create(int, const char*const*) {
		Tcl::instance().add_errorf("Cannot create instance of "
					   "AddrParamsClass");
		return NULL;
	}
	virtual void bind();
	virtual int method(int ac, const char*const* av);

	static inline AddrParamsClass& instance() { return *instance_; }

	inline int mcast_shift() { return McastShift_; }
	inline int mcast_mask() { return McastMask_; }
	inline int port_shift() { return PortShift_; }
	inline int port_mask() { return PortMask_; }
	inline int node_shift(int n) { return NodeShift_[n]; }
	inline int node_mask(int n) { return NodeMask_[n]; }
	inline int hlevel() { return hlevel_; }
	inline int nodebits() { return nodebits_; }

private:
	int McastShift_;
	int McastMask_;
	int PortShift_;
	int PortMask_;
	int hlevel_;
	int *NodeMask_;
	int *NodeShift_;
	int nodebits_;
	static AddrParamsClass *instance_;
};

#endif // ns_addr_params_h
