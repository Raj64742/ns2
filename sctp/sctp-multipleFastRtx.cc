/*
 * Copyright (c) 2001-2003 by the Protocol Engineering Lab, U of Delaware
 * All rights reserved.
 *
 * Armando L. Caro Jr. <acaro@@cis,udel,edu>
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
 */

/* MultipleFastRtx extension implements the Caro Multiple Fast Retransmit
 * Algorithm. Caro's Algorithm introduces a fastRtxRecover state variable
 * per TSN in the send buffer. Any time a TSN is retransmitted, its
 * fastRtxRecover is set to the highest TSN outstanding at the time of
 * retransmit. That way, only missing reports triggered by TSNs beyond
 * fastRtxRecover may trigger yet another fast retransmit.
 */

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/sctp/sctp-multipleFastRtx.cc,v 1.1 2003/08/21 18:29:14 haldar Exp $ (UD/PEL)";
#endif

#include "ip.h"
#include "sctp-multipleFastRtx.h"
#include "flags.h"
#include "random.h"
#include "template.h"

#include "sctpDebug.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define	MIN(x,y)	(((x)<(y))?(x):(y))
#define	MAX(x,y)	(((x)>(y))?(x):(y))

static class MultipleFastRtxSctpClass : public TclClass 
{ 
public:
  MultipleFastRtxSctpClass() : TclClass("Agent/SCTP/MultipleFastRtx") {}
  TclObject* create(int, const char*const*) 
  {
    return (new MultipleFastRtxSctpAgent());
  }
} classSctpMultipleFastRtx;

MultipleFastRtxSctpAgent::MultipleFastRtxSctpAgent() : SctpAgent()
{
}

void MultipleFastRtxSctpAgent::delay_bind_init_all()
{
  SctpAgent::delay_bind_init_all();
}

int MultipleFastRtxSctpAgent::delay_bind_dispatch(const char *cpVarName, 
					  const char *cpLocalName, 
					  TclObject *opTracer)
{
  return SctpAgent::delay_bind_dispatch(cpVarName, cpLocalName, opTracer);
}

void MultipleFastRtxSctpAgent::AddToSendBuffer(SctpDataChunkHdr_S *spChunk, 
				int iChunkSize,
				u_int uiReliability,
				SctpDest_S *spDest)
{
  DBG_I(AddToSendBuffer);
  DBG_PL(AddToSendBuffer, "spDest=%p  iChunkSize=%d"), 
    spDest, iChunkSize DBG_PR;

  Node_S *spNewNode = new Node_S;
  spNewNode->eType = NODE_TYPE_SEND_BUFFER;
  spNewNode->vpData = new SctpSendBufferNode_S;

  SctpSendBufferNode_S * spNewNodeData 
    = (SctpSendBufferNode_S *) spNewNode->vpData;

  /* This can NOT simply be a 'new SctpDataChunkHdr_S', because we need to
   * allocate the space for the ENTIRE data chunk and not just the data
   * chunk header.  
   */
  spNewNodeData->spChunk = (SctpDataChunkHdr_S *) new u_char[iChunkSize];
  memcpy(spNewNodeData->spChunk, spChunk, iChunkSize);

  spNewNodeData->eAdvancedAcked = FALSE;
  spNewNodeData->eGapAcked = FALSE;
  spNewNodeData->eAddedToPartialBytesAcked = FALSE;
  spNewNodeData->iNumMissingReports = 0;
  spNewNodeData->iUnrelRtxLimit = uiReliability;
  spNewNodeData->eMarkedForRtx = FALSE;
  spNewNodeData->eIneligibleForFastRtx = FALSE;
  spNewNodeData->iNumTxs = 1;
  spNewNodeData->spDest = spDest;

  // BEGIN -- MultipleFastRtx changes to this function  
  spNewNodeData->uiFastRtxRecover = 0;
  // END -- MultipleFastRtx changes to this function  

  /* Is there already a DATA chunk in flight measuring an RTT? 
   * (6.3.1.C4 RTT measured once per round trip)
   */
  if(spDest->eRtoPending == FALSE)  // NO?
    {
      spNewNodeData->dTxTimestamp = Scheduler::instance().clock();
      spDest->eRtoPending = TRUE;   // ...well now there is :-)
    }
  else
    spNewNodeData->dTxTimestamp = 0; // don't use this check for RTT estimate

  InsertNode(&sSendBuffer, sSendBuffer.spTail, spNewNode, NULL);

  DBG_X(AddToSendBuffer);
}

/* Go thru the send buffer deleting all chunks which have a tsn <= the 
 * tsn parameter passed in. We assume the chunks in the rtx list are ordered by 
 * their tsn value. In addtion, for each chunk deleted:
 *   1. we add the chunk length to # newly acked bytes and partial bytes acked
 *   2. we update round trip time if appropriate
 *   3. stop the timer if the chunk's destination timer is running
 */
void MultipleFastRtxSctpAgent::SendBufferDequeueUpTo(u_int uiTsn)
{
  DBG_I(SendBufferDequeueUpTo);

  Node_S *spDeleteNode = NULL;
  Node_S *spCurrNode = sSendBuffer.spHead;
  SctpSendBufferNode_S *spCurrNodeData = NULL;
  Boolean_E eSkipRttUpdate = FALSE;

  /* Only the first TSN that is being dequeued can be used to reset the
   * error cunter on a destination. Why? Well, suppose there are some
   * chunks that were gap acked before the primary had errors. Then the
   * gap gets filled with a retransmission using an alternate path. The
   * filled gap will cause the cum ack to move past the gap acked TSNs,
   * but does not mean that the they can reset the errors on the primary.
   */
  
  iAssocErrorCount = 0;

  spCurrNodeData = (SctpSendBufferNode_S *) spCurrNode->vpData;

  /* trigger trace ONLY if it was previously NOT 0 */
  if(spCurrNodeData->spDest->iErrorCount != 0)
    {
      spCurrNodeData->spDest->iErrorCount = 0; // clear error counter
      tiErrorCount++;                          // ... and trace it too!
      spCurrNodeData->spDest->eStatus = SCTP_DEST_STATUS_ACTIVE;
      if(spCurrNodeData->spDest == spPrimaryDest &&
	 spNewTxDest != spPrimaryDest) 
	{
	  DBG_PL(SendBufferDequeueUpTo,
		 "primary recovered... migrating back from %p to %p"),
	    spNewTxDest, spPrimaryDest DBG_PR;
	  spNewTxDest = spPrimaryDest; // return to primary
	}
    }

  while(spCurrNode != NULL &&
	((SctpSendBufferNode_S*)spCurrNode->vpData)->spChunk->uiTsn <= uiTsn)
    {
      spCurrNodeData = (SctpSendBufferNode_S *) spCurrNode->vpData;

      /* Only count this chunk as newly acked and towards partial bytes
       * acked if it hasn't been gap acked or marked as ack'd due to rtx
       * limit.  
       */
      if((spCurrNodeData->eGapAcked == FALSE) &&
	 (spCurrNodeData->eAdvancedAcked == FALSE) )
	{
	  spCurrNodeData->spDest->iNumNewlyAckedBytes 
	    += spCurrNodeData->spChunk->sHdr.usLength;

	  /* only add to partial bytes acked if we are in congestion
	   * avoidance mode and if there was cwnd amount of data
	   * outstanding on the destination (implementor's guide) 
	   */
	  if(spCurrNodeData->spDest->iCwnd >spCurrNodeData->spDest->iSsthresh &&
	     ( spCurrNodeData->spDest->iOutstandingBytes 
	       >= spCurrNodeData->spDest->iCwnd) )
	    {
	      spCurrNodeData->spDest->iPartialBytesAcked 
		+= spCurrNodeData->spChunk->sHdr.usLength;
	    }
	}


      // BEGIN -- MultipleFastRtx changes to this function  

      /* This is to ensure that Max.Burst is applied when a SACK
       * acknowledges a chunk which has been fast retransmitted. (If the
       * fast rtx recover variable is set, that can only be because it was
       * fast rtxed.) This is a proposed change to RFC2960 section 7.2.4
       */
      if(spCurrNodeData->uiFastRtxRecover > 0)
	eApplyMaxBurst = TRUE;

      // END -- MultipleFastRtx changes to this function  

	    
      /* We update the RTT estimate if the following hold true:
       *   1. RTO pending flag is set (6.3.1.C4 measured once per round trip)
       *   2. Timestamp is set for this chunk (ie, we were measuring this chunk)
       *   3. This chunk has not been retransmitted
       *   4. This chunk has not been gap acked already 
       *   5. This chunk has not been advanced acked (pr-sctp: exhausted rtxs)
       */
      if(spCurrNodeData->spDest->eRtoPending == TRUE &&
	 spCurrNodeData->dTxTimestamp > 0 &&
	 spCurrNodeData->iNumTxs == 1 &&
	 spCurrNodeData->eGapAcked == FALSE &&
	 spCurrNodeData->eAdvancedAcked == FALSE) 
	{
	  RttUpdate(spCurrNodeData->dTxTimestamp, spCurrNodeData->spDest);
	  spCurrNodeData->spDest->eRtoPending = FALSE;
	}

      /* if there is a timer running on the chunk's destination, then stop it
       */
      if(spCurrNodeData->spDest->eRtxTimerIsRunning == TRUE)
	StopT3RtxTimer(spCurrNodeData->spDest);

      spDeleteNode = spCurrNode;
      spCurrNode = spCurrNode->spNext;
      DeleteNode(&sSendBuffer, spDeleteNode);
      spDeleteNode = NULL;
    }

  DBG_X(SendBufferDequeueUpTo);
}

/* returns a boolean of whether a fast retransmit is necessary
 */
Boolean_E MultipleFastRtxSctpAgent::ProcessGapAckBlocks(u_char *ucpSackChunk,
					 Boolean_E eNewCumAck)
{
  DBG_I(ProcessGapAckBlocks);

  Boolean_E eFastRtxNeeded = FALSE;
  u_int uiHighestTsnNewlySacked = uiCumAckPoint; // fast rtx (impl guide v.02)
  u_int uiStartTsn;
  u_int uiEndTsn;
  Node_S *spCurrNode = NULL;
  SctpSendBufferNode_S *spCurrNodeData = NULL;
  Node_S *spCurrDestNode = NULL;
  SctpDest_S *spCurrDestNodeData = NULL;
  Boolean_E eFirstOutstanding = FALSE;  

  SctpSackChunk_S *spSackChunk = (SctpSackChunk_S *) ucpSackChunk;

  u_short usNumGapAcksProcessed = 0;
  SctpGapAckBlock_S *spCurrGapAck 
    = (SctpGapAckBlock_S *) (ucpSackChunk + sizeof(SctpSackChunk_S));

  // BEGIN -- MultipleFastRtx changes to this function    
  u_int uiHighestOutstandingTsn
    = ((SctpSendBufferNode_S *) sSendBuffer.spTail->vpData)->spChunk->uiTsn;
  // END -- MultipleFastRtx changes to this function    

  DBG_PL(ProcessGapAckBlocks,"CumAck=%d"), spSackChunk->uiCumAck DBG_PR;

  if(sSendBuffer.spHead == NULL) // do we have ANYTHING in the rtx buffer?
    {
      /* This COULD mean that this sack arrived late, and a previous one
       * already cum ack'd everything. ...so, what do we do? nothing??
       */
    }
  
  else // we do have chunks in the rtx buffer
    {
      /* make sure we clear all the eSeenFirstOutstanding flags before
       * using them!  
       */
      for(spCurrDestNode = sDestList.spHead;
	  spCurrDestNode != NULL;
	  spCurrDestNode = spCurrDestNode->spNext)
	{
	  spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;
	  spCurrDestNodeData->eSeenFirstOutstanding = FALSE;
	}

      for(spCurrNode = sSendBuffer.spHead;
	  (spCurrNode != NULL) &&
	    (usNumGapAcksProcessed != spSackChunk->usNumGapAckBlocks);
	  spCurrNode = spCurrNode->spNext)
	{
	  spCurrNodeData = (SctpSendBufferNode_S *) spCurrNode->vpData;

	  DBG_PL(ProcessGapAckBlocks, "eSeenFirstOutstanding=%s"),
	    spCurrNodeData->spDest->eSeenFirstOutstanding ? "TRUE" : "FALSE"
	    DBG_PR;
		 
	  /* is this chunk the first outstanding on its destination?
	   */
	  if(spCurrNodeData->spDest->eSeenFirstOutstanding == FALSE &&
	     spCurrNodeData->eGapAcked == FALSE &&
	     spCurrNodeData->eAdvancedAcked == FALSE)
	    {
	      /* yes, it is the first!
	       */
	      eFirstOutstanding = TRUE;
	      spCurrNodeData->spDest->eSeenFirstOutstanding = TRUE;
	    }
	  else
	    {
	      /* nope, not the first...
	       */
	      eFirstOutstanding = FALSE;
	    }

	  DBG_PL(ProcessGapAckBlocks, "eFirstOutstanding=%s"),
	    eFirstOutstanding ? "TRUE" : "FALSE" DBG_PR;

	  DBG_PL(ProcessGapAckBlocks, "--> rtx list chunk begin") DBG_PR;

	  DBG_PL(ProcessGapAckBlocks, "    TSN=%d"), 
	    spCurrNodeData->spChunk->uiTsn 
	    DBG_PR;

	  DBG_PL(ProcessGapAckBlocks, "    %s=%s %s=%s"),
	    "eGapAcked", 
	    spCurrNodeData->eGapAcked ? "TRUE" : "FALSE",
	    "eAddedToPartialBytesAcked",
	    spCurrNodeData->eAddedToPartialBytesAcked ? "TRUE" : "FALSE" 
	    DBG_PR;

	  DBG_PL(ProcessGapAckBlocks, "    NumMissingReports=%d NumTxs=%d"),
	    spCurrNodeData->iNumMissingReports, 
	    spCurrNodeData->iNumTxs 
	    DBG_PR;

	  DBG_PL(ProcessGapAckBlocks, "<-- rtx list chunk end") DBG_PR;
	  
	  DBG_PL(ProcessGapAckBlocks,"GapAckBlock StartOffset=%d EndOffset=%d"),
	    spCurrGapAck->usStartOffset, spCurrGapAck->usEndOffset DBG_PR;

	  uiStartTsn = spSackChunk->uiCumAck + spCurrGapAck->usStartOffset;
	  uiEndTsn = spSackChunk->uiCumAck + spCurrGapAck->usEndOffset;
	  
	  DBG_PL(ProcessGapAckBlocks, "GapAckBlock StartTsn=%d EndTsn=%d"),
	    uiStartTsn, uiEndTsn DBG_PR;

	  if(spCurrNodeData->spChunk->uiTsn < uiStartTsn)
	    {
	      /* This chunk is NOT being acked and is missing at the receiver
	       */

	      /* If this chunk was GapAcked before, then either the
	       * receiver has renegged the chunk (which our simulation
	       * doesn't do) or this SACK is arriving out of order.
	       */
	      if(spCurrNodeData->eGapAcked == TRUE)
		{
		  DBG_PL(ProcessGapAckBlocks, 
			 "out of order SACK? setting TSN=%d eGapAcked=FALSE"),
		    spCurrNodeData->spChunk->uiTsn DBG_PR;
		  spCurrNodeData->eGapAcked = FALSE;
		  spCurrNodeData->spDest->iOutstandingBytes 
		    += spCurrNodeData->spChunk->sHdr.usLength;

		  /* section 6.3.2.R4 says that we should restart the
		   * T3-rtx timer here if it isn't running already. In our
		   * implementation, it isn't necessary since
		   * ProcessSackChunk will restart the timer for any
		   * destinations which have outstanding data and don't
		   * have a timer running.
		   */
		}
	    }
	  else if((uiStartTsn <= spCurrNodeData->spChunk->uiTsn) && 
		  (spCurrNodeData->spChunk->uiTsn <= uiEndTsn) )
	    {
	      /* This chunk is being acked via a gap ack block
	       */
	      DBG_PL(ProcessGapAckBlocks, "gap ack acks this chunk: %s%s"),
		"eGapAcked=",
		spCurrNodeData->eGapAcked ? "TRUE" : "FALSE" 
		DBG_PR;

	      if(spCurrNodeData->eGapAcked == FALSE)
		{
		  DBG_PL(ProcessGapAckBlocks, "setting eGapAcked=TRUE") DBG_PR;
		  spCurrNodeData->eGapAcked = TRUE;
		  spCurrNodeData->eMarkedForRtx = FALSE; // unmark

		  /* modified fast rtx algorithm (implementor's guide v.02)
		   */
		  if(uiHighestTsnNewlySacked < spCurrNodeData->spChunk->uiTsn)
		    uiHighestTsnNewlySacked = spCurrNodeData->spChunk->uiTsn;

		  if(spCurrNodeData->eAdvancedAcked == FALSE)
		    {
		      spCurrNodeData->spDest->iNumNewlyAckedBytes 
			+= spCurrNodeData->spChunk->sHdr.usLength;
		    }

		  /* only increment partial bytes acked if we are in
		   * congestion avoidance mode, we have a new cum ack, and
		   * we haven't already incremented it for this sack
		   */
		  if(( spCurrNodeData->spDest->iCwnd 
		       > spCurrNodeData->spDest->iSsthresh) &&
		     eNewCumAck == TRUE &&
		     spCurrNodeData->eAddedToPartialBytesAcked == FALSE)
		    {
		      DBG_PL(ProcessGapAckBlocks, 
			     "setting eAddedToPartiallyBytesAcked=TRUE") DBG_PR;

		      spCurrNodeData->eAddedToPartialBytesAcked = TRUE; // set

		      spCurrNodeData->spDest->iPartialBytesAcked 
			+= spCurrNodeData->spChunk->sHdr.usLength;
		    }
		  /* We update the RTT estimate if the following hold true:
		   *   1. RTO pending flag is set (6.3.1.C4)
		   *   2. Timestamp is set for this chunk 
		   *   3. This chunk has not been retransmitted
		   *   4. This chunk has not been gap acked already 
		   *   5. This chunk has not been advanced acked (pr-sctp)
		   */
		  if(spCurrNodeData->spDest->eRtoPending == TRUE &&
		     spCurrNodeData->dTxTimestamp > 0 &&
		     spCurrNodeData->iNumTxs == 1 &&
		     spCurrNodeData->eAdvancedAcked == FALSE) 
		    {
		      RttUpdate(spCurrNodeData->dTxTimestamp, 
				spCurrNodeData->spDest);
		      spCurrNodeData->spDest->eRtoPending = FALSE;
		    }

		  /* section 6.3.2.R3 - Stop the timer if this is the
		   * first outstanding for this destination (note: it may
		   * have already been stopped if there was a new cum
		   * ack). If there are still outstanding bytes on this
		   * destination, we'll restart the timer later in
		   * ProcessSackChunk() 
		   */
		  if(eFirstOutstanding == TRUE 
		     && spCurrNodeData->spDest->eRtxTimerIsRunning == TRUE)
		    StopT3RtxTimer(spCurrNodeData->spDest);
		  
		  iAssocErrorCount = 0;
		  
		  /* trigger trace ONLY if it was previously NOT 0
		   */
		  if(spCurrNodeData->spDest->iErrorCount != 0)
		    {
		      spCurrNodeData->spDest->iErrorCount = 0; // clear errors
		      tiErrorCount++;                       // ... and trace it!
		      spCurrNodeData->spDest->eStatus = SCTP_DEST_STATUS_ACTIVE;
		      if(spCurrNodeData->spDest == spPrimaryDest &&
			 spNewTxDest != spPrimaryDest) 
			{
			  DBG_PL(ProcessGapAckBlocks,
				 "primary recovered... "
				 "migrating back from %p to %p"),
			    spNewTxDest, spPrimaryDest DBG_PR;
			  spNewTxDest = spPrimaryDest; // return to primary
			}
		    }
		}
	    }
	  else if(spCurrNodeData->spChunk->uiTsn > uiEndTsn)
	    {
	      /* This point in the rtx buffer is already past the tsns which are
	       * being acked by this gap ack block.  
	       */
	      usNumGapAcksProcessed++; 

	      /* Did we process all the gap ack blocks?
	       */
	      if(usNumGapAcksProcessed != spSackChunk->usNumGapAckBlocks)
		{
		  DBG_PL(ProcessGapAckBlocks, "jump to next gap ack block") 
		    DBG_PR;

		  spCurrGapAck 
		    = ((SctpGapAckBlock_S *)
		       (ucpSackChunk + sizeof(SctpSackChunk_S)
			+ (usNumGapAcksProcessed * sizeof(SctpGapAckBlock_S))));
		}

	      /* If this chunk was GapAcked before, then either the
	       * receiver has renegged the chunk (which our simulation
	       * doesn't do) or this SACK is arriving out of order.
	       */
	      if(spCurrNodeData->eGapAcked == TRUE)
		{
		  DBG_PL(ProcessGapAckBlocks, 
			 "out of order SACK? setting TSN=%d eGapAcked=FALSE"),
		    spCurrNodeData->spChunk->uiTsn DBG_PR;
		  spCurrNodeData->eGapAcked = FALSE;
		  spCurrNodeData->spDest->iOutstandingBytes 
		    += spCurrNodeData->spChunk->sHdr.usLength;
		  
		  /* section 6.3.2.R4 says that we should restart the
		   * T3-rtx timer here if it isn't running already. In our
		   * implementation, it isn't necessary since
		   * ProcessSackChunk will restart the timer for any
		   * destinations which have outstanding data and don't
		   * have a timer running.
		   */
		}
	    }
	}

      /* By this time, either we have run through the entire send buffer or we
       * have run out of gap ack blocks. In the case that we have run out of gap
       * ack blocks before we finished running through the send buffer, we need
       * to mark the remaining chunks in the send buffer as eGapAcked=FALSE.
       * This final marking needs to be done, because we only trust gap ack info
       * from the last SACK. Otherwise, renegging (which we don't do) or out of
       * order SACKs would give the sender an incorrect view of the peer's rwnd.
       */
      for(; spCurrNode != NULL; spCurrNode = spCurrNode->spNext)
	{
	  /* This chunk is NOT being acked and is missing at the receiver
	   */
	  spCurrNodeData = (SctpSendBufferNode_S *) spCurrNode->vpData;

	  /* If this chunk was GapAcked before, then either the
	   * receiver has renegged the chunk (which our simulation
	   * doesn't do) or this SACK is arriving out of order.
	   */
	  if(spCurrNodeData->eGapAcked == TRUE)
	    {
	      DBG_PL(ProcessGapAckBlocks, 
		     "out of order SACK? setting TSN=%d eGapAcked=FALSE"),
		spCurrNodeData->spChunk->uiTsn DBG_PR;
	      spCurrNodeData->eGapAcked = FALSE;
	      spCurrNodeData->spDest->iOutstandingBytes 
		+= spCurrNodeData->spChunk->sHdr.usLength;

	      /* section 6.3.2.R4 says that we should restart the T3-rtx
	       * timer here if it isn't running already. In our
	       * implementation, it isn't necessary since ProcessSackChunk
	       * will restart the timer for any destinations which have
	       * outstanding data and don't have a timer running.
	       */
	    }
	}

      DBG_PL(ProcessGapAckBlocks, "now incrementing missing reports...") DBG_PR;
      DBG_PL(ProcessGapAckBlocks, "uiHighestTsnNewlySacked=%d"), 
	     uiHighestTsnNewlySacked DBG_PR;

      for(spCurrNode = sSendBuffer.spHead;
	  spCurrNode != NULL; 
	  spCurrNode = spCurrNode->spNext)
	{
	  spCurrNodeData = (SctpSendBufferNode_S *) spCurrNode->vpData;

	  DBG_PL(ProcessGapAckBlocks, "TSN=%d eGapAcked=%s"), 
	    spCurrNodeData->spChunk->uiTsn,
	    spCurrNodeData->eGapAcked ? "TRUE" : "FALSE"
	    DBG_PR;

	  if(spCurrNodeData->eGapAcked == FALSE)
	    {
	      // BEGIN -- MultipleFastRtx changes to this function  

	      /* Caro's Multiple Fast Rtx Algorithm
	       */
	      if(spCurrNodeData->spChunk->uiTsn < uiHighestTsnNewlySacked &&
		 uiHighestTsnNewlySacked > spCurrNodeData->uiFastRtxRecover)
		{
		  spCurrNodeData->iNumMissingReports++;
		  DBG_PL(ProcessGapAckBlocks, 
			 "incrementing missing report for TSN=%d to %d"), 
		    spCurrNodeData->spChunk->uiTsn,
		    spCurrNodeData->iNumMissingReports
		    DBG_PR;

		  if(spCurrNodeData->iNumMissingReports == FAST_RTX_TRIGGER &&
		     spCurrNodeData->eAdvancedAcked == FALSE)
		    {
		      spCurrNodeData->iNumMissingReports = 0;
		      MarkChunkForRtx(spCurrNodeData);
		      eFastRtxNeeded = TRUE;
		      spCurrNodeData->uiFastRtxRecover =uiHighestOutstandingTsn;
			
		      DBG_PL(ProcessGapAckBlocks, 
			     "resetting missing report for TSN=%d to 0"), 
			spCurrNodeData->spChunk->uiTsn
			DBG_PR;
		      DBG_PL(ProcessGapAckBlocks, 
			     "setting uiFastRtxRecover=%d"), 
			spCurrNodeData->uiFastRtxRecover
			DBG_PR;
		    }
		}
	      // END -- MultipleFastRtx changes to this function  
	    }
	}
    }

  DBG_PL(ProcessGapAckBlocks, "eFastRtxNeeded=%s"), 
    eFastRtxNeeded ? "TRUE" : "FALSE" DBG_PR;
  DBG_X(ProcessGapAckBlocks);
  return eFastRtxNeeded;
}
