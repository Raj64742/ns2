// CMU-Monarch project's Mobility extensions ported by Padma Haldar, 
// 10/98.


#include <phy.h>
#include <wired-phy.h>
#include <node.h>



static class NodeClass : public TclClass {
public:
	NodeClass() : TclClass("Node") {}
	TclObject* create(int, const char*const*) {
                return (new Node);
        }
} class_node;

Node::Node(void)
{
	LIST_INIT(&ifhead_);	
}

int
Node::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if(strncasecmp(argv[1], "addif", 5) == 0) {
			WiredPhy* n = (WiredPhy*) TclObject::lookup(argv[2]);
			if(n == 0)
				return TCL_ERROR;
			n->insertnode(&ifhead_);
			n->setnode(this);
			return TCL_OK;
		}
	}
	return TclObject::command(argc,argv);
}



#ifdef zero	
static class HierNodeClass : public TclClass {
public:
	HierNodeClass() : TclClass("HierNode") {}
	TclObject* create(int, const char*const*) {
                return (new HierNode);
        }
} class_hier_node;

static class ManualRtNodeClass : public TclClass {
public:
	ManualRtNodeClass() : TclClass("ManualRtNode") {}
	TclObject* create(int, const char*const*) {
                return (new ManualRtNode);
        }
} class_ManualRt_node;

static class VirtualClassifierNodeClass : public TclClass {
public:
	VirtualClassifierNodeClass() : TclClass("VirtualClassifierNode") {}
	TclObject* create(int, const char*const*) {
                return (new VirtualClassifierNode);
        }
} class_VirtualClassifier_node;
#endif

