#include "agg-spec.h"

#include "ip.h"
#include "ident-tree.h"

AggSpec::AggSpec(int dstON, int dstPrefix, int dstBits) {

  dstON_ = dstON;
  dstPrefix_ = dstPrefix;
  dstBits_ = dstBits;
  
  ptype_=-1;
  ptypeShare_=0;
}

AggSpec::AggSpec(AggSpec * orig) {
  dstON_ = orig->dstON_;
  dstPrefix_ = orig->dstPrefix_;
  dstBits_ = orig->dstBits_;

  ptype_ = orig->ptype_;
  ptypeShare_=orig->ptypeShare_;
}

int
AggSpec::member(Packet * pkt) {
  
  hdr_ip * iph = hdr_ip::access(pkt);
  ns_addr_t dst = iph->dst();

  if (dstON_) {
    int prefix = getPrefix(dst.addr_);
    if (prefix == dstPrefix_) {
      return 1;
    }
  }

#ifdef DEBUG_AS
  printf("AS: non-member packet with dst %d at %g. Agg: ", dst.addr_, Scheduler::instance().clock());
  print();
#endif
  return 0;
}

int 
AggSpec::getPrefix(int addr) {
  
  int andAgent = ((1 << dstBits_) - 1) << (NO_BITS - dstBits_);
  return (addr &  andAgent);
}
 
int 
AggSpec::equals(AggSpec * another) {
  
  return (dstON_ == another->dstON_ && dstPrefix_ == another->dstPrefix_ && 
	  dstBits_ == another->dstBits_);
}

int 
AggSpec::contains(AggSpec * another) {
  
  if (another->dstBits_ < dstBits_) return 0; 
  if (dstON_ != another->dstON_) return 0;

  int prefix1 = PrefixTree::getPrefixBits(dstPrefix_, dstBits_);
  int prefix2 = PrefixTree::getPrefixBits(another->dstPrefix_, dstBits_);
  
  return (prefix1 == prefix2);
}

void 
AggSpec::expand(int prefix, int bits) {
  
  dstPrefix_ = prefix;
  dstBits_ = bits;
}

int 
AggSpec::subsetOfDst(int prefix, int bits) {
  
  if (!dstON_ || dstBits_ < bits) return 0;

  int myPrefix = PrefixTree::getPrefixBits(dstPrefix_, bits);
  return (prefix==myPrefix);
}

AggSpec *
AggSpec::clone() {
  return new AggSpec(this);
}

int 
AggSpec::getSampleAddress() {
  
  //for now, return the prefix itself
  return dstPrefix_;
}

int 
AggSpec::prefixBitsForMerger(AggSpec * agg1, AggSpec * agg2) {
  
  int bitsNow = (agg1->dstBits_ < agg2->dstBits_)? agg1->dstBits_: agg2->dstBits_;
  for (int i=bitsNow; i>=0; i--) {
    int prefix1 = PrefixTree::getPrefixBits(agg1->dstPrefix_, i);
    int prefix2 = PrefixTree::getPrefixBits(agg2->dstPrefix_, i);
    if (prefix1==prefix2) {
      return i;
    }
  }

  printf("AS: Error: Should never come here\n");
  exit(-1);
}

void 
AggSpec::print() {
  printf("Prefix = %d Bits = %d\n", dstPrefix_, dstBits_);
}
  
