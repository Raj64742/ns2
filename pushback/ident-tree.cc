#include "ident-tree.h"
#include "rate-limit.h"

// ############################ PrefixTree Methods ####################

PrefixTree::PrefixTree() {

  for (int i=0; i<=getLastIndex(); i++) {
    countArray[i]=0;
  }
} 

void
PrefixTree::reset() {

  for (int i=0; i<=getLastIndex(); i++) {
    countArray[i]=0;
  }
}

void
PrefixTree::traverse() {
  printf("Traversal \n");
  for (int i=0; i<=getLastIndex(); i++) {
    printf("%d/%d %d\n",getPrefixFromIndex(i), getNoBitsFromIndex(i), countArray[i]);
  }
}

void
PrefixTree::registerDrop(int address, int size) {

  if (address > getMaxAddress()) {
    printf("ERROR: Address out of Range\n");
    exit(EXIT_FAILURE);
  }

  for (int i=0; i<=NO_BITS; i++) {
    int index = getIndexFromAddress(address, i);
    countArray[index]+=size;
  }
}

AggReturn *  
PrefixTree::identifyAggregate(double arrRate, double linkBW) {
  
  int sum = 0; int count=0;
  for (int i=getFirstIndexOfBit(NO_BITS); i<=getLastIndexOfBit(NO_BITS); i++) {
    if (countArray[i]!=0) {
      sum+=countArray[i];
      count++;
    }
  }

  if (count == 0) return NULL;

  cluster *clusterList = (cluster *)malloc(sizeof(cluster)*MAX_CLUSTER);
  
  for (int i=0; i < MAX_CLUSTER; i++) {
    clusterList[i].prefix_=-1;
    clusterList[i].count_=0;
  }
  
  double mean = sum/count;
  for (int i=getFirstIndexOfBit(NO_BITS); i<=getLastIndexOfBit(NO_BITS); i++) {
    if (countArray[i] >= mean/2) { //using mean/2 helps in trivial simulations.
      insertCluster(clusterList, i, countArray[i], CLUSTER_LEVEL);
    }
  }
  
  int i=0;
  for (; i<MAX_CLUSTER; i++) {
    if (clusterList[i].prefix_==-1) {
      break;
    }
    goDownCluster(clusterList, i);
  }
  int lastIndex = i-1;
  
  sortCluster(clusterList, lastIndex);
  
  //check for natural rifts here, if you want to.
  
  double targetRate = linkBW/(1 - TARGET_DROPRATE);
  double excessRate = arrRate - targetRate;
  double rateTillNow = 0;
  double requiredBottom;
  int id=0;
  for (; id<=lastIndex; id++) {
    rateTillNow+=clusterList[id].count_*(arrRate/countArray[0]);
    requiredBottom = (rateTillNow - excessRate)/(id+1);
    if (clusterList[id+1].prefix_==-1) {
      break;
    }
    if (clusterList[id+1].count_* (arrRate/countArray[0]) < requiredBottom) break;
  }

  return new AggReturn(clusterList, requiredBottom, id, countArray[0]);
}
    
void
PrefixTree::insertCluster(cluster * clusterList, int index, int count, int noBits) {
  
  int address = getPrefixFromIndex(index);
  int prefix = (address >> (NO_BITS - noBits)) << (NO_BITS - noBits);
  int breakCode=0;
  for (int i=0;i<MAX_CLUSTER; i++) {
    if (clusterList[i].prefix_ == prefix && clusterList[i].bits_ == noBits) {
      clusterList[i].count_+=count;
      breakCode=1;
      break;
    }
    if (clusterList[i].prefix_ == -1) {
      clusterList[i].prefix_ = prefix;
      clusterList[i].bits_ = noBits;
      clusterList[i].count_=count;
      breakCode=2;
      break;
    }
  }
  
  if (breakCode==0) {
    printf("Error: Too Small MAX_CLUSTER. Increase and Recompile\n");
    exit(-1);
  }
}

void
PrefixTree::goDownCluster(cluster * clusterList, int index) {
  
  int noBits = clusterList[index].bits_;
  int prefix = clusterList[index].prefix_;
  
  int treeIndex = getIndexFromPrefix(prefix, noBits);
  int maxIndex = treeIndex;
  while (1) {
    int leftIndex = 2*maxIndex+1;
    if (leftIndex > getLastIndex()) break;
    int leftCount = countArray[leftIndex];
    int rightCount = countArray[leftIndex+1];
    if (leftCount > 9*rightCount) {
      maxIndex = leftIndex;
    } 
    else if (rightCount > 9*leftCount) {
      maxIndex = leftIndex+1;
    }
    else {
      break;
    }
  }

  clusterList[index].prefix_=getPrefixFromIndex(maxIndex);
  clusterList[index].bits_=getNoBitsFromIndex(maxIndex);
  clusterList[index].count_=countArray[maxIndex];
}

void
PrefixTree::sortCluster(cluster * clusterList, int lastIndex) {
  
  for (int i=0; i<=lastIndex; i++) {
    for (int j=i+1; j<=lastIndex; j++) {
      if (clusterList[i].count_ < clusterList[j].count_) {
	swapCluster(clusterList, i, j);
      }
    }
  }
}
 
void 
PrefixTree::swapCluster(cluster * clusterList, int id1, int id2) {
  
  int tempP = clusterList[id1].prefix_;
  int tempB = clusterList[id1].bits_;
  int tempC = clusterList[id1].count_;

  clusterList[id1].prefix_ = clusterList[id2].prefix_;
  clusterList[id1].bits_   = clusterList[id2].bits_;
  clusterList[id1].count_  = clusterList[id2].count_;

  clusterList[id2].prefix_ = tempP;
  clusterList[id2].bits_   = tempB;
  clusterList[id2].count_  = tempC;
}

int
PrefixTree::getMaxAddress() {
  return (1 << NO_BITS) - 1;
}

int
PrefixTree::getBitI(int address, int i) {

  int andAgent = 1 << (NO_BITS - i);
  int bitI = address & andAgent;
  if (bitI) 
    return 1;
  else 
    return 0;
}

int
PrefixTree::getIndexFromPrefix(int prefix, int noBits) {
  int base = (1 << noBits) - 1;
  return base + (prefix >> (NO_BITS - noBits));
}

int
PrefixTree::getIndexFromAddress(int address, int noBits) {
  
  int base = (1 << noBits) - 1;
//   int andAgent = address >> (NO_BITS - noBits);
//   int additive = base & andAgent;
  int additive = address >> (NO_BITS - noBits);
    
  return base + additive;
}

int 
PrefixTree::getPrefixFromIndex(int index) {
  
  int noBits = getNoBitsFromIndex(index);
  int base = (1 << noBits) - 1;
  int prefix = (index - base) << (NO_BITS - noBits);
    
  return prefix;
}


int 
PrefixTree::getPrefixBits(int prefix, int noBits) {
  return (prefix >> (NO_BITS - noBits)) << (NO_BITS - noBits);
}
  
int 
PrefixTree::getNoBitsFromIndex(int index) {
 
  //using 1.2 is an ugly hack to get precise floating point calculation.
  int noBits = (int) floor(log(index+1.2)/log(2));
  return noBits;
}

int 
PrefixTree::getFirstIndexOfBit(int noBits) {
  return ( 1 << noBits) - 1;
}

int 
PrefixTree::getLastIndexOfBit(int noBits) {
  return ( 1 << (noBits+1)) - 2;
}

// ######################## IdentStruct Methods ########################

IdentStruct::IdentStruct() {
  dstTree_ = new PrefixTree();
  srcTree_ = new PrefixTree();
  dropHash_ = new DropHashTable();
}

void
IdentStruct::reset() {
  dstTree_->reset();
  srcTree_->reset();
  //  dropHash_->reset();
}
  
void 
IdentStruct::traverse() {
  dstTree_->traverse();
  srcTree_->traverse();
  //  dropHash_->traverse();
}

void 
IdentStruct::registerDrop(Packet *p) {
   
  hdr_ip * iph = hdr_ip::access(p);
  ns_addr_t src = iph->src();
  ns_addr_t dst = iph->dst();
  // int fid = iph->flowid();
  
  hdr_cmn* hdr  = HDR_CMN(p);
  int size = hdr->size();

  dstTree_->registerDrop(dst.addr_, size);
  srcTree_->registerDrop(src.addr_, size);
  //  dropHash_->insert(p, size);
}

AggReturn * 
IdentStruct::identifyAggregate(double arrRate, double linkBW) {
  return dstTree_->identifyAggregate(arrRate, linkBW);
}

