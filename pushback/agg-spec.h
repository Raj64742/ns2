#ifndef ns_agg_spec_h
#define ns_agg_spec_h

#include "pushback-constants.h"
#include "packet.h"

class AggSpec {
 public:

  int dstON_;         //whether destination based pushback is ON.
  int dstPrefix_;     //destination prefix  
  int dstBits_; //number of bits in the prefix;

  //other dimensions of rate-limiting go here;
  int ptype_;           //packet type, which is to be rate-limited.
  double ptypeShare_;

  AggSpec(int dstON, int dstP, int dstBits);
  AggSpec(AggSpec * orig);

  int member(Packet *p);
  int getPrefix(int addr);
  int equals(AggSpec *);
  int contains(AggSpec *);
  int subsetOfDst(int prefix, int bits);
  void expand(int prefix, int bits);
  AggSpec* clone();
  int getSampleAddress();
  static int prefixBitsForMerger(AggSpec *, AggSpec *);
  void print();
};
  
#endif
