/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) @ Regents of the University of California.
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
 * 	This product includes software developed by the MASH Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/scoreboard.h,v 1.7 1998/06/27 01:24:45 gnguyen Exp $
 */

#ifndef ns_scoreboard_h
#define ns_scoreboard_h

//  Definition of the scoreboard class:

#define SBSIZE 1024

#include "tcp.h"

class ScoreBoard {
  public:
	ScoreBoard() {first_=0; length_=0;}
	int IsEmpty () {return (length_ == 0);}
	void ClearScoreBoard (); 
	int GetNextRetran ();
	void MarkRetran (int retran_seqno);
	void MarkRetran (int retran_seqno, int snd_nxt);
	int UpdateScoreBoard (int last_ack_, hdr_tcp*);
	int CheckSndNxt (hdr_tcp*);
	
  protected:
	int first_, length_;
	struct ScoreBoardNode {
		int seq_no_;		/* Packet number */
		int ack_flag_;		/* Acked by cumulative ACK */
		int sack_flag_;		/* Acked by SACK block */
		int retran_;		/* Packet retransmitted */
		int snd_nxt_;		/* snd_nxt at time of retransmission */
		int sack_cnt_;		/* number of reports for this hole */
	} SBN[SBSIZE+1];
};

#endif
