/*
 * A RED queue that allows certain operations to be done in a way
 * that depends on higher-layer semantics. The only such operation
 * at present is pickPacketToDrop(), which invokes the corresponding
 * member function in SemanticPacketQueue.
 */

#include "red.h"
#include "semantic-packetqueue.h"

class SemanticREDQueue : public REDQueue {
public:
	SemanticREDQueue() : REDQueue() {}
	Packet* pickPacketToDrop() {
		return(((SemanticPacketQueue*) q_)->pickPacketToDrop());
	}
};

static class SemanticREDClass : public TclClass {
public:
	SemanticREDClass() : TclClass("Queue/RED/Semantic") {}
	TclObject* create(int, const char*const*) {
		return (new SemanticREDQueue);
	}
} class_semantic_red;

