// CMU-Monarch project's Mobility extensions ported by Padma Haldar, 
// 10/98.

#ifndef __node_h__
#define __node_h__

#include "connector.h"
#include "object.h"
#include "phy.h"



// srm-topo.h already uses class Node- hence the odd name "Node"
//
// For now, this is a dummy split object for class Node. In future, we
// may move somethings over from OTcl.
//

class Node : public TclObject {
 public:
	Node(void);
	virtual int command(int argc, const char*const* argv);
	//apparently the subclasses cannot access private variable of the superclass 
	struct if_head ifhead_;
 private:
	//struct if_head ifhead_;
	
};

#ifdef zero
class HierNode : public Node {
public:
	HierNode() {}    
};

class ManualRtNode : public Node {
public:
	ManualRtNode() {}    
};

class VirtualClassifierNode : public Node {
public:
	VirtualClassifierNode() {}    
};
#endif

#endif /* __node_h__ */



