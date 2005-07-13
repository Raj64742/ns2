/*
 * Copyright (c) 2001-2003 by the Protocol Engineering Lab, U of Delaware
 * All rights reserved.
 *
 * Armando L. Caro Jr. <acaro@@cis,udel,edu>
 * Janardhan Iyengar   <iyengar@@cis,udel,edu>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/sctp/sctp.h,v 1.3 2005/07/13 03:51:27 tomh Exp $ (UD/PEL)
 */

#ifndef ns_sctp_h
#define ns_sctp_h

#include "agent.h"
#include "node.h"
#include "packet.h"

/* The SCTP Common header is 12 bytes.
 */
#define SCTP_HDR_SIZE         12

#define MAX_RWND_SIZE         0xffffffff
#define MAX_DATA_CHUNK_SIZE   0xffffffff
#define MIN_DATA_CHUNK_SIZE   16
#define MAX_NUM_STREAMS       0x0000ffff

#define DELAYED_SACK_TRIGGER  2      // sack for every 2 data packets
#define SACK_GEN_TIMEOUT      0.200  // in seconds

#define FAST_RTX_TRIGGER      4
#define INITIAL_RTO           3      // in seconds
#define MIN_RTO               1      // in seconds
#define MAX_RTO               60     // in seconds
#define RTO_ALPHA             0.125  // RTO.alpha is 1/8
#define RTO_BETA              0.25   // RTO.Beta is 1/4

#define MAX_BURST             4
typedef enum MaxBurstUsage_E
{
  MAX_BURST_USAGE_OFF,      // 0
  MAX_BURST_USAGE_ON        // 1
};

/* Let us use OUR typedef'd enum for FALSE and TRUE. It's much better this way.
 * ...why? because NOW all boolean variables can be explicitly declared as
 * such. There is no longer any ambiguity between a regular int variable and a
 * boolean variable.  
 */
#undef FALSE
#undef TRUE
typedef enum 
{
	FALSE,
	TRUE
}Boolean_E;

/* Who controls the data sending, app layer or the transport layer 
 * (as in the case of infinite data)
 */
typedef enum DataSource_E
{
  DATA_SOURCE_APPLICATION,
  DATA_SOURCE_INFINITE
};

/* SCTP chunk types 
 */
typedef enum
{
	SCTP_CHUNK_DATA,
	SCTP_CHUNK_INIT,
	SCTP_CHUNK_INIT_ACK,
	SCTP_CHUNK_SACK,
	SCTP_CHUNK_HB,
	SCTP_CHUNK_HB_ACK,
	SCTP_CHUNK_ABORT,
	SCTP_CHUNK_SHUTDOWN,
	SCTP_CHUNK_SHUTDOWN_ACK,
	SCTP_CHUNK_ERROR,
	SCTP_CHUNK_COOKIE_ECHO,
	SCTP_CHUNK_COOKIE_ACK,
	SCTP_CHUNK_ECNE,                  // reserved in rfc2960
	SCTP_CHUNK_CWR,                   // reserved in rfc2960
	SCTP_CHUNK_SHUTDOWN_COMPLETE,
	
	/* RFC2960 leaves room for later defined chunks */
	/* for U-SCTP/PR-SCTP
	 */
	SCTP_CHUNK_FORWARD_TSN,    // should be 192, but this is a simulation! :-)
	/* for timestamp option (sctp-timestamp.cc)
	 */
	SCTP_CHUNK_TIMESTAMP
}SctpChunkType_E;

typedef struct AppData_S 
{
  /* Parameters needed for establishing an association 
   */
	u_short usNumStreams;     // Number of streams to associate
	u_short usNumUnreliable;  // first usNumUnreliable streams will be unreliable
	
	/* Parameters needed for app data messages 
	 */
	u_short    usStreamId;     // Which stream does this message go on?
	u_short    usReliability;  // What level of relability does this message have
	Boolean_E  eUnordered;     // Is this an unordered message?
	u_int      uiNumBytes;     // Number of databytes in message
};

/* ns specific header fields used for tracing SCTP traffic
 * (This was done so that the 'trace' module wouldn't have to look into the
 * payload of SCTP packets)
 */
typedef struct SctpTrace_S
{
	SctpChunkType_E  eType;
	u_int            uiTsn;    // (cum ack for sacks, -1 for other control chunks)
	u_short          usStreamId;     // -1 for control chunks
	u_short          usStreamSeqNum; // -1 for control chunks
};

struct hdr_sctp
{
	/* ns required header fields/methods 
	 */
	static int offset_;	// offset for this header
	inline static int& offset() { return offset_; }
	inline static hdr_sctp* access(Packet* p) 
	{
		return (hdr_sctp*) p->access(offset_);
	}
	
	/* ns specific header fields used for tracing SCTP traffic
	 * (This was done so that the 'trace' module wouldn't have to look into the
	 * payload of SCTP packets)
	 */
	u_int         uiNumChunks;
	SctpTrace_S  *spSctpTrace;
	
	u_int&        NumChunks() { return uiNumChunks; }
	SctpTrace_S*& SctpTrace() { return spSctpTrace; }
};

typedef struct SctpChunkHdr_S
{
	u_char  ucType;
	u_char  ucFlags;
	u_short usLength;
};

/* INIT paramater types
 */
#define SCTP_INIT_PARAM_UNREL  0xC000
typedef struct SctpUnrelStreamsParam_S
{
	u_short  usType;
	u_short  usLength;
	
	/* unreliable stream start-end pairs are appended dynamically
	 */
};

typedef struct SctpUnrelStreamPair_S
{
	u_short  usStart;
	u_short  usEnd;
};

typedef struct SctpInitChunk_S  // this is used for init ack, too 
{
	SctpChunkHdr_S  sHdr;
	u_int           uiInitTag;		 // tag of mine (not used)
	u_int           uiArwnd; 	         // referred to as a_rwnd in rfc2960
	u_short         usNumOutboundStreams;	 // referred to as OS in rfc2960
	u_short         usMaxInboundStreams;   // referred to as MIS in rfc2960
	u_int           uiInitialTsn;          
	
	SctpUnrelStreamsParam_S  sUnrelStream;	
};
typedef SctpInitChunk_S SctpInitAckChunk_S;

typedef struct SctpCookieEchoChunk_S
{
	SctpChunkHdr_S  sHdr;
	
	/* cookie would go here, but we aren't implementing this at the moment */
};
typedef SctpCookieEchoChunk_S SctpCookieAckChunk_S;

typedef struct SctpDataChunkHdr_S
{
	SctpChunkHdr_S  sHdr;
	u_int           uiTsn;
	u_short         usStreamId;
	u_short         usStreamSeqNum;
	u_int           uiPayloadType;     // not used
	
  /* user data must be appended dynamically when filling packets */
};

/* Data Chunk Specific Flags
 */
#define SCTP_DATA_FLAG_END        0x01  // indicates last fragment
#define SCTP_DATA_FLAG_BEGINNING  0x02  // indicates first fragment
#define SCTP_DATA_FLAG_UNORDERED  0x04  // indicates unordered DATA chunk

/* SACK has the following structure and following it will be some number of
 * gap acks and duplicate tsns.
 */
typedef struct SctpSackChunk_S
{
	SctpChunkHdr_S  sHdr;
	u_int           uiCumAck;
	u_int           uiArwnd;
	u_short         usNumGapAckBlocks;
	u_short         usNumDupTsns;
	
  /* Gap Ack Blocks and Duplicate TSNs are appended dynamically
   */
};

typedef struct SctpGapAckBlock_S
{
	u_short  usStartOffset;
	u_short  usEndOffset;
};

typedef struct SctpDupTsn_S
{
	u_int  uiTsn;
};

#define SCTP_CHUNK_FORWARD_TSN_LENGTH  8
typedef struct SctpForwardTsnChunk_S
{
	SctpChunkHdr_S  sHdr;
	u_int           uiNewCum;
};

typedef struct SctpDest_S;
typedef struct SctpHeartbeatChunk_S
{
	SctpChunkHdr_S  sHdr;
	u_short         usInfoType;      // filled in, but not really used
	u_short         usInfoLength;    // filled in, but not really used
	double          dTimestamp;
	SctpDest_S     *spDest;
	u_long          ulDummyField;   // only to ensure 2-byte alignment for SPARC
};
typedef SctpHeartbeatChunk_S SctpHeartbeatAckChunk_S;

/* SCTP state defines for internal state machine */
typedef enum SctpState_E
{
	SCTP_STATE_UNINITIALIZED,
	SCTP_STATE_CLOSED,
	SCTP_STATE_ESTABLISHED,
	SCTP_STATE_COOKIE_WAIT,
	SCTP_STATE_COOKIE_ECHOED,
	SCTP_STATE_SHUTDOWN_SENT,        // not currently used
	SCTP_STATE_SHUTDOWN_RECEIVED,    // not currently used
	SCTP_STATE_SHUTDOWN_ACK_SENT,    // not currently used
	SCTP_STATE_SHUTDOWN_PENDING      // not currently used
};

class SctpAgent;

class T1InitTimer : public TimerHandler 
{
 public:
	T1InitTimer(SctpAgent *a) : TimerHandler(), opAgent(a) { }
	
 protected:
	virtual void expire(Event *);
	SctpAgent *opAgent;
};

class T1CookieTimer : public TimerHandler 
{
public:
	T1CookieTimer(SctpAgent *a) : TimerHandler(), opAgent(a) { }
	
 protected:
	virtual void expire(Event *);
	SctpAgent *opAgent;
};

class T3RtxTimer : public TimerHandler 
{
public:
	T3RtxTimer(SctpAgent *a, SctpDest_S *d) 
		: TimerHandler(), opAgent(a) {spDest = d;}
	
 protected:
	virtual void expire(Event *);
	SctpAgent  *opAgent;
	SctpDest_S *spDest;  // destination this timer corresponds to
};

class CwndDegradeTimer : public TimerHandler 
{
public:
	CwndDegradeTimer(SctpAgent *a, SctpDest_S *d) 
		: TimerHandler(), opAgent(a) {spDest = d;}
	
 protected:
	virtual void expire(Event *);
	SctpAgent  *opAgent;
	SctpDest_S *spDest;  // destination this timer corresponds to
};

class HeartbeatGenTimer : public TimerHandler 
{
public:
  HeartbeatGenTimer(SctpAgent *a, SctpDest_S *d) 
    : TimerHandler(), opAgent(a) {spDest = d;}

  double      dStartTime; // timestamp of when timer started
	
protected:
  virtual void expire(Event *);
  SctpAgent  *opAgent;
  SctpDest_S *spDest;     // destination this timer corresponds to
};

class HeartbeatTimeoutTimer : public TimerHandler 
{
public:
  HeartbeatTimeoutTimer(SctpAgent *a, SctpDest_S *d) 
    : TimerHandler(), opAgent(a) {spDest = d;}
	
  SctpDest_S *spDest;  // destination this timer corresponds to

protected:
  virtual void expire(Event *);
  SctpAgent  *opAgent;
};

typedef struct SctpInterface_S
{
  int        iNsAddr;
  int        iNsPort;
  NsObject  *opTarget;
  NsObject  *opLink;
};

typedef enum SctpDestStatus_E
{
  SCTP_DEST_STATUS_INACTIVE,
  SCTP_DEST_STATUS_ACTIVE
};

typedef struct SctpDest_S
{
  int        iNsAddr;  // ns "IP address"
  int        iNsPort;  // ns "port"

  /* Packet is simply used for determing src addr. The header stores dest addr,
   * which makes it easy to determine src target using
   * Connector->find(Packet *). Then with the target, we can determine src 
   * addr.
   */
  Packet    *opRoutingAssistPacket;

  u_int         uiCwnd;                // current congestion window
  u_int         uiSsthresh;            // current ssthresh value
  Boolean_E     eFirstRttMeasurement; // is this our first RTT measurement?
  double        dRto;                 // current retransmission timeout value
  double        dSrtt;                // current smoothed round trip time
  double        dRttVar;              // current RTT variance
  int           iPmtu;                // current known path MTU (not used)
  Boolean_E     eRtxTimerIsRunning;   // is there a timer running already?
  T3RtxTimer   *opT3RtxTimer;         // retransmission timer
  Boolean_E     eRtoPending;          // DATA chunk being used to measure RTT?

  u_int  uiPartialBytesAcked; // helps to modify cwnd in congestion avoidance mode
  u_int  uiOutstandingBytes;  // outstanding bytes still in flight (unacked)

  u_int                   uiErrorCount;             // destination error counter
  SctpDestStatus_E        eStatus;                 // active/inactive
  CwndDegradeTimer       *opCwndDegradeTimer;      // timer to degrade cwnd
  double                  dIdleSince;              // timestamp since idle
  HeartbeatGenTimer      *opHeartbeatGenTimer;     // to trigger a heartbeat
  HeartbeatTimeoutTimer  *opHeartbeatTimeoutTimer; // heartbeat timeout timer

  /* these are temporary variables needed per destination and they should
   * be cleared for each usage.  
   */
  Boolean_E  eCcApplied;            // congestion control already applied?
  Boolean_E  eSeenFirstOutstanding; // have we seen the first outstanding yet?
  u_int      uiNumNewlyAckedBytes;   // counts newly ack'd bytes in a sack
};

typedef enum NodeType_E
{
  NODE_TYPE_STREAM_BUFFER,
  NODE_TYPE_RECV_TSN_BLOCK,
  NODE_TYPE_DUP_TSN,
  NODE_TYPE_SEND_BUFFER,
  NODE_TYPE_APP_LAYER_BUFFER,
  NODE_TYPE_INTERFACE_LIST,
  NODE_TYPE_DESTINATION_LIST
};

typedef struct Node_S
{
  NodeType_E  eType;
  void       *vpData;   // u can put any data type into the node
  Node_S     *spNext;
  Node_S     *spPrev;
};

typedef struct List_S
{
  u_int    uiLength;
  Node_S  *spHead;
  Node_S  *spTail;
};

typedef struct SctpRecvTsnBlock_S
{
  u_int  uiStartTsn;
  u_int  uiEndTsn;
};

/* Each time the sender retransmits marked chunks, how many can be sent? Well,
 * immediately after a timeout or when the 4 missing report is received, only
 * one packet of retransmits may be sent. Later, the number of packets is gated
 * by cwnd
 */
typedef enum SctpRtxLimit_E
{
  RTX_LIMIT_ONE_PACKET,
  RTX_LIMIT_CWND
};

typedef struct SctpSendBufferNode_S
{
  SctpDataChunkHdr_S  *spChunk;
  Boolean_E            eAdvancedAcked;     // acked via rtx expiration (u-sctp)
  Boolean_E            eGapAcked;          // acked via a Gap Ack Block
  Boolean_E            eAddedToPartialBytesAcked; // already accounted for?
  int                  iNumMissingReports; // # times reported missing
  int                  iUnrelRtxLimit;     // limit on # of unreliable rtx's
  Boolean_E            eMarkedForRtx;      // has it been marked for rtx?
  Boolean_E            eIneligibleForFastRtx; // ineligible for fast rtx??
  int                  iNumTxs;            // # of times transmitted (orig+rtx)
  double               dTxTimestamp;  
  SctpDest_S          *spDest;             // destination last sent to

  /* variables used for extensions
   */
  u_int                uiFastRtxRecover;   // sctp-multipleFastRtxs.cc uses this
};

typedef struct SctpStreamBufferNode_S
{
  SctpDataChunkHdr_S  *spChunk;
};

typedef enum SctpStreamMode_E
{
  SCTP_STREAM_RELIABLE,
  SCTP_STREAM_UNRELIABLE
};

typedef struct SctpInStream_S
{
  SctpStreamMode_E  eMode;
  u_short           usNextStreamSeqNum;
  List_S            sBufferedChunkList;
};

typedef struct SctpOutStream_S
{
  SctpStreamMode_E  eMode;
  u_short           usNextStreamSeqNum;	
};

class SackGenTimer : public TimerHandler 
{
public:
  SackGenTimer(SctpAgent *a) : TimerHandler(), opAgent(a) { }
	
protected:
  virtual void expire(Event *);
  SctpAgent *opAgent;
};

class SctpAgent : public Agent 
{
public:
  SctpAgent();
  ~SctpAgent();
	
  virtual void  recv(Packet *pkt, Handler*);
  virtual void  sendmsg(int nbytes, const char *flags = 0);
  virtual int   command(int argc, const char*const* argv);
	
  void          T1InitTimerExpiration();
  void          T1CookieTimerExpiration();
  virtual void  Timeout(SctpChunkType_E, SctpDest_S *);
  void          CwndDegradeTimerExpiration(SctpDest_S *);
  void          HeartbeatGenTimerExpiration(double, SctpDest_S *);
  void          SackGenTimerExpiration();

protected:
  virtual void  delay_bind_init_all();
  virtual int   delay_bind_dispatch(const char *varName, const char *localName,
				    TclObject *tracer);

  /* initialization stuff
   */
  void           SetDebugOutFile();
  void           Reset();
  virtual void   OptionReset();
  virtual u_int  ControlChunkReservation();

  /* tracing functions
   */
  void       TraceVar(const char*);
  void       TraceAll();
  void       trace(TracedVar*);

  /* generic list functions
   */
  void       InsertNode(List_S *, Node_S *, Node_S *, Node_S *);
  void       DeleteNode(List_S *, Node_S *);
  void       ClearList(List_S *);

  /* multihome functions
   */
  void       AddInterface(int, int, NsObject *, NsObject *);
  void       AddDestination(int, int);
  int        SetPrimary(int);
  int        ForceSource(int);

  /* chunk generation functions
   */
  u_int        GenChunk(SctpChunkType_E, u_char *);
  u_int        GetNextDataChunkSize();
  int          GenOneDataChunk(u_char *);
  int          GenMultipleDataChunks(u_char *, int);
  virtual u_int  BundleControlChunks(u_char *);

  /* sending functions
   */
  void          StartT3RtxTimer(SctpDest_S *);
  void          StopT3RtxTimer(SctpDest_S *);
  virtual void  AddToSendBuffer(SctpDataChunkHdr_S *, int, u_int, SctpDest_S *);
  void          RttUpdate(double, SctpDest_S *);
  virtual void  SendBufferDequeueUpTo(u_int);
  void          AdjustCwnd(SctpDest_S *);
  void          AdvancePeerAckPoint();
  virtual void  FastRtx();
  void          TimeoutRtx(SctpDest_S *);
  void          MarkChunkForRtx(SctpSendBufferNode_S *);
  Boolean_E     AnyMarkedChunks();
  virtual void  RtxMarkedChunks(SctpRtxLimit_E);
  void          SendHeartbeat(SctpDest_S *);
  SctpDest_S   *GetNextDest(SctpDest_S *);
  double        CalcHeartbeatTime(double);
  void          SetSource(SctpDest_S *);
  void          SetDestination(SctpDest_S *);
  void          SendPacket(u_char *, int, SctpDest_S *);
  SctpDest_S   *GetReplyDestination(hdr_ip *);
  u_int         TotalOutstanding();
  void          SendMuch();

  /* receiving functions
   */
  Boolean_E  UpdateHighestTsn(u_int);
  Boolean_E  IsDuplicateChunk(u_int);
  void       InsertDuplicateTsn(u_int);
  void       UpdateCumAck();
  void       UpdateRecvTsnBlocks(u_int);
  void       PassToUpperLayer(SctpDataChunkHdr_S *);
  void       InsertInStreamBuffer(List_S *, SctpDataChunkHdr_S *);
  void       PassToStream(SctpDataChunkHdr_S *);
  void       UpdateAllStreams();

  /* processing functions
   */
  void               ProcessInitChunk(u_char *);
  void               ProcessInitAckChunk(u_char *);
  void               ProcessCookieEchoChunk(SctpCookieEchoChunk_S *);
  void               ProcessCookieAckChunk(SctpCookieAckChunk_S *);
  void               ProcessDataChunk(SctpDataChunkHdr_S *);
  virtual Boolean_E  ProcessGapAckBlocks(u_char *, Boolean_E);
  virtual void       ProcessSackChunk(u_char *); /* @@@ */
  void               ProcessForwardTsnChunk(SctpForwardTsnChunk_S *);  
  void               ProcessHeartbeatAckChunk(SctpHeartbeatChunk_S *);  
  virtual void       ProcessOptionChunk(u_char *);
  int                ProcessChunk(u_char *, u_char **);
  void               NextChunk(u_char **, int *);

  /* misc functions
   */
  void       Close();

  /* debugging functions
   */
  void SctpAgent::DumpSendBuffer();

  /* sctp association state variable
   */
  SctpState_E     eState;

  /* App Layer buffer
   */
  List_S          sAppLayerBuffer;

  /* multihome variables
   */
  Classifier         *opCoreTarget;
  List_S              sInterfaceList;
  List_S              sDestList;
  SctpDest_S         *spPrimaryDest;       // primary destination
  SctpDest_S         *spNewTxDest;         // destination for new transmissions
  SctpDest_S         *spReplyDest; // reply with sacks or control chunk replies
  Boolean_E           eForceSource;
  u_int               uiAssocErrorCount;  // total error counter for the assoc

  /* heartbeat variables
   */
  HeartbeatGenTimer      *opHeartbeatGenTimer;      // to trigger a heartbeat
  HeartbeatTimeoutTimer  *opHeartbeatTimeoutTimer;  // heartbeat timeout timer

  /* sending variables
   */
  T1InitTimer       *opT1InitTimer;    // T1-init timer
  T1CookieTimer     *opT1CookieTimer;  // T1-cookie timer
  u_int              uiInitTryCount;    // # of unsuccessful INIT attempts
  u_int              uiNextTsn;
  u_short            usNextStreamId; // used to round-robin the streams
  SctpOutStream_S   *spOutStreams;
  u_int              uiPeerRwnd;
  u_int              uiCumAckPoint;  
  u_int              uiAdvancedPeerAckPoint;
  List_S             sSendBuffer;
  Boolean_E          eForwardTsnNeeded;  // is a FORWARD TSN chunk needed?
  Boolean_E          eSendNewDataChunks; // should we send new data chunks too?
  Boolean_E          eMarkedChunksPending; // chunks waiting to be rtx'd?
  Boolean_E          eApplyMaxBurst; 
  DataSource_E       eDataSource;
  u_int              uiBurstLength;  // tracks sending burst per SACK

  /* receiving variables
   */
  u_int            uiMyRwnd;
  u_int            uiCumAck;       
  u_int            uiHighestRecvTsn; // higest tsn recv'd of entire assoc
  List_S           sRecvTsnBlockList;
  List_S           sDupTsnList;
  int              iNumInStreams;
  SctpInStream_S  *spInStreams;
  Boolean_E        eStartOfPacket;             // for delayed sack triggering
  int              iDataPktCountSinceLastSack; // for delayed sack triggering
  Boolean_E        eSackChunkNeeded; // do we need to transmit a sack chunk?
  SackGenTimer    *opSackGenTimer;    // sack generation timer

  /* tcl bindable variables
   */
  u_int            uiDebugMask;     // 32 bits for fine level debugging
  int              iDebugFileIndex; // 1 debug output file per agent 
  u_int            uiPathMaxRetrans;
  u_int            uiAssociationMaxRetrans;
  u_int            uiMaxInitRetransmits;
  Boolean_E        eOneHeartbeatTimer;  // one heartbeat timer for all dests?
  u_int            uiHeartbeatInterval;
  u_int            uiMtu;
  u_int            uiInitialRwnd;
  u_int            uiInitialSsthresh;
  u_int            uiIpHeaderSize;
  u_int            uiDataChunkSize;
  u_int            uiNumOutStreams;
  Boolean_E        eUseDelayedSacks; // are we using delayed sacks?
  MaxBurstUsage_E  eUseMaxBurst;
  u_int            uiInitialCwnd;
  u_int            uiNumUnrelStreams;
  u_int            uiReliability; // k-rtx on all chunks & all unrel streams
  Boolean_E        eUnordered;    // sets for all chunks on all streams :-(
  Boolean_E        eRtxToAlt;     // rtxs to alternate destination?
  Boolean_E        eTraceAll;     // trace all variables on one line?
  TracedInt        tiCwnd;        // trace cwnd for all destinations
  TracedDouble     tdRto;         // trace rto for all destinations
  TracedInt        tiErrorCount;  // trace error count for all destinations

  /* globally used non-tcl bindable variables, but rely on the tcl bindable
   */
  u_int           uiMaxPayloadSize; // we don't want this to be tcl bindable
  u_int           uiMaxDataSize; // max payload size - reserved control bytes
  FILE           *fhpDebugFile; 	   // file pointer for debugging output

  /* tracing variables that will be copied into hdr_sctp
   */
  u_int           uiNumChunks;
  SctpTrace_S    *spSctpTrace;  
};

#endif
