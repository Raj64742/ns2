/*
 * Copyright (c) 1996 The Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
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


/* 9/96 Pittsburgh Supercomputing Center
 *      UpdateScoreBoard, CheckSndNxt, MarkRetran modified for fack
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/scoreboard.cc,v 1.5 1997/03/29 02:43:11 tomh Exp $ (LBL)";
#endif

/*  A quick hack version of the scoreboard  */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <math.h>

#include "packet.h"
#include "scoreboard.h"
#include "tcp.h"

#define ASSERT(x) if (!(x)) {printf ("Assert SB failed\n"); exit(1);}
#define ASSERT1(x) if (!(x)) {printf ("Assert1 SB (length)\n"); exit(1);}

#define SBNI SBN[i%SBSIZE]

// last_ack = TCP last ack
int ScoreBoard::UpdateScoreBoard (int last_ack, Packet *pkt)
{
	int i, sack_index, sack_left, sack_right;
	int retran_decr = 0;

	//  If there is no scoreboard, create one.
	if (length_ == 0) {
		i = last_ack+1;
		SBNI.seq_no_ = i;
		SBNI.ack_flag_ = 0;
		SBNI.sack_flag_ = 0;
		SBNI.retran_ = 0;
		SBNI.snd_nxt_ = 0;
		first_ = i%SBSIZE;
		length_++;
		if (length_ >= SBSIZE) {
			printf ("Error, scoreboard too large\n");
			exit(1);
		}
	}	

	hdr_tcp *tcph = TCPHeader::access(pkt->bits());
	for (sack_index=0; sack_index < tcph->sa_length(); sack_index++) {
		sack_left = tcph->sa_left()[sack_index];
		sack_right = tcph->sa_right()[sack_index];

		//  Create new entries off the right side.
		if (sack_right > SBN[(first_+length_+SBSIZE-1)%SBSIZE].seq_no_) {
			//  Create new entries
			for (i = SBN[(first_+length_+SBSIZE-1)%SBSIZE].seq_no_+1; i<sack_right; i++) {
				SBNI.seq_no_ = i;
				SBNI.ack_flag_ = 0;
				SBNI.sack_flag_ = 0;
				SBNI.retran_ = 0;
				SBNI.snd_nxt_ = 0;
				length_++;
				if (length_ >= SBSIZE) {
					printf ("Error, scoreboard too large\n");
					exit(1);
				}
			}
		}

		//  Advance the left edge of the block.
		if (SBN[first_].seq_no_ <= last_ack) {
			for (i=SBN[(first_)%SBSIZE].seq_no_; i<=last_ack; i++) {
				//  Advance the ACK
				if (SBNI.seq_no_ <= last_ack) {
					ASSERT(first_ == i%SBSIZE);
					first_ = (first_+1)%SBSIZE; 
					length_--;
					ASSERT1(length_ >= 0);
					SBNI.ack_flag_ = 1;
					SBNI.sack_flag_ = 1;
					if (SBNI.retran_) {
						SBNI.retran_ = 0;
						SBNI.snd_nxt_ = 0;
						retran_decr++;
					}
					if (length_==0) 
					  break;
				}
			}
		}
		
		for (i=SBN[(first_)%SBSIZE].seq_no_; i<sack_right; i++) {
			//  Check to see if this segment is now covered by the sack block
			if (SBNI.seq_no_ >= sack_left && SBNI.seq_no_ < sack_right) {
				if (! SBNI.sack_flag_) {
					SBNI.sack_flag_ = 1;
				}
				if (SBNI.retran_) {
				  SBNI.retran_ = 0;
				  retran_decr++;
				}
			}
		}
	}
	return (retran_decr);
}
int ScoreBoard::CheckSndNxt (Packet *pkt)
{
	int i, sack_index, sack_left, sack_right;
	int force_timeout = 0;

	hdr_tcp *tcph = TCPHeader::access(pkt->bits());
	for (sack_index=0; sack_index < tcph->sa_length(); sack_index++) {
		sack_left = tcph->sa_left()[sack_index];
		sack_right = tcph->sa_right()[sack_index];

		for (i=SBN[(first_)%SBSIZE].seq_no_; i<sack_right; i++) {
			//  Check to see if this segment's snd_nxt_ is now covered by the sack block
			if (SBNI.retran_ && SBNI.snd_nxt_ < sack_right) {
			        // the packet was lost again
				SBNI.retran_ = 0;
				SBNI.snd_nxt_ = 0;
				force_timeout = 1;
			}
		}
	}
	return (force_timeout);
}

void ScoreBoard::ClearScoreBoard()
{
	length_ = 0;
}

/*
 * GetNextRetran() returns "-1" if there is no packet that is
 *   not acked and not sacked and not retransmitted.
 */
int ScoreBoard::GetNextRetran()	    // Returns sequence number of next pkt...
{
	int i;

	if (length_) {
	      for (i=SBN[(first_)%SBSIZE].seq_no_; 
		   i<SBN[(first_)%SBSIZE].seq_no_+length_; i++) {
		    if (!SBNI.ack_flag_ && !SBNI.sack_flag_ && !SBNI.retran_) {
			  return (i);
		    }
	      }
	}
	return (-1);
}


void ScoreBoard::MarkRetran (int retran_seqno, int snd_nxt)
{
	SBN[retran_seqno%SBSIZE].retran_ = 1;
	SBN[retran_seqno%SBSIZE].snd_nxt_ = snd_nxt;
}

void ScoreBoard::MarkRetran (int retran_seqno)
{
	SBN[retran_seqno%SBSIZE].retran_ = 1;
}

