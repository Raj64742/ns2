/*
 * (c) 1997 StarBurst Communications Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author: Christoph Haenle, chris@cs.vu.nl
 * File: mftp_snd.cc
 * Last change: Jan. 13, 1998
 *
 * This software may freely be used only for non-commercial purposes
 */

#include <stdlib.h>     // strtoul, etc.
#include <assert.h>
#include <string.h>

#include <stdio.h>

#include "Tcl.h"
#include "agent.h"
#include "packet.h"
#include "ip.h"
#include "mftp_snd.h"
#include "trace.h"
#include "bitops.h"       // due to IS_BITSET, etc.

#define min(a, b)       ((a) < (b) ? (a) : (b))

static class MFTPSndAgentClass : public TclClass {
public:
    MFTPSndAgentClass() : TclClass("Agent/MFTP/Snd") {}
    TclObject* create(int, const char*const*) {
        return (new MFTPSndAgent());
    }
} class_mftpsnd_agent;


static class MFTPHeaderClass : public PacketHeaderClass {
public:
    MFTPHeaderClass() : PacketHeaderClass("PacketHeader/MFTP",
                                          sizeof(hdr_mftp)) {}
} class_mftphdr;


MFTPSndAgent::MFTPSndAgent()
    : MFTPAgent(),
      nak_bytes(0),
      last_nak(0),
      last_nak_mask(0),
      naks(0),
      retx(0),
      fseek_offset(0),
      read_ahead_bufsize(0)
{
    bind("readAheadBufsize_", &readAheadBufsize_);
    bind_time("txStatusDelay_", &txStatusDelay_);
    bind("nakCount_", &nakCount_);
}

int MFTPSndAgent::command(int argc, const char*const* argv)
{
    Tcl& tcl = Tcl::instance();

    if(strcmp(argv[1], "send") == 0) {
        if(strcmp(argv[2], "data") == 0) {
            send_data();
            return TCL_OK;
        } else if(strcmp(argv[2], "statreq") == 0) {
            sb_ulong pass_nb, block_lo, block_hi;
            int nb_scanned = 0;
            if(argc == 6) {
                nb_scanned += sscanf(argv[3], "%ul", &pass_nb);
                nb_scanned += sscanf(argv[4], "%ul", &block_lo);
                nb_scanned += sscanf(argv[5], "%ul", &block_hi);
            }
            if(nb_scanned != 3) {
                tcl.resultf("%s: wrong number of parameters for \"send statreq\"", name_);
                return TCL_ERROR;
            }
            send_status_request(pass_nb, block_lo, block_hi, txStatusDelay_); 
            return TCL_OK;
        }
    }
    if(strcmp(argv[1], "start") == 0) {
        if(MFTPAgent::init() == TCL_ERROR) {
            return TCL_ERROR;
        };
        user_file_init(fileSize_, dtuSize_,
                       dtusPerBlock_, dtusPerGroup_,
                       readAheadBufsize_);
        return TCL_OK;
    }
    return Agent::command(argc, argv);
}

void MFTPSndAgent::recv(Packet* p, Handler* h)
{
    hdr_ip* ih = (hdr_ip*) p->access(off_ip_);
    hdr_mftp* mh = (hdr_mftp*) p->access(off_mftp_);

    if(ih->dst() == 0) {
        // Packet from local agent.
        assert(false);
    } else {
        switch(mh->type) {
        case hdr_mftp::PDU_DATA_TRANSFER:
        case hdr_mftp::PDU_STATUS_REQUEST:
            /* as we are a member of the group as well, we receive all data */
            /* we have sent. */
            break;
        case hdr_mftp::PDU_NAK:
            process_nak(mh->spec.nak, p->accessdata(), nmstats.CurrentPass-1); /* -1 because we have */
            /* incremented the pass-number already in send_data. */
            break;
        default:
            assert(false);
        }
        Packet::free(p);
    }
}

sb_void MFTPSndAgent::send_status_request(sb_ulong pass_nb, sb_ulong block_lo, sb_ulong block_hi, double txStatusDelay)
{
    Packet* p = Agent::allocpkt();
    hdr_mftp* hdr = (hdr_mftp*) p->access(off_mftp_);

    assert(FileDGrams > 0);                    // we need this requirement here

    /* GENERATE THE HEADER */
    hdr->type = hdr_mftp::PDU_STATUS_REQUEST;
    hdr->spec.statReq.pass_nb  = pass_nb;
    hdr->spec.statReq.block_lo = block_lo;
    hdr->spec.statReq.block_hi = block_hi;
    hdr->spec.statReq.group_lo = 0;
    hdr->spec.statReq.group_hi = (block_lo == block_hi && block_hi == nb_blocks() ? FileDGrams-1 : dtus_per_block-1);
    hdr->spec.statReq.RspBackoffWindow = txStatusDelay / 2;
    // @@@ this is still in error (must be dependent on group-size). 
    // Anyway, group_lo and group_hi are currently not used...

    /* transmit packet */
    hdr_cmn* ch = (hdr_cmn*) p->access(off_cmn_);
    ch->size() = sizeof(hdr_mftp);
    target_->recv(p);
}


// process_nak: process incoming nak
sb_void MFTPSndAgent::process_nak(hdr_mftp::Spec::Nak& nak, sb_uint8* nak_bitmap, sb_ulong currentPass)
{
    assert(1 <= nak.nak_count && nak.nak_count <= nb_groups);

    Tcl& tcl = Tcl::instance();
    tcl.evalf("%s recv-nak %u %u %u", name_, (unsigned) nak.pass_nb, 
              (unsigned) nak.block_nb, (unsigned) nak.nak_count);

    /* PASS GREATER THAN EXPECTED, IGNORE BECAUSE IT'S A BOGUS STATUS RESPONSE! */
    if(nak.pass_nb > currentPass) {
        assert(false);               // this assert-statement should go away later.
        return;
    }

    assert(dtus_per_block % 8 == 0);         // we need this for the following.

    /* start_group_nb corresponds to first bit of NAK-bitmap: */
    sb_ulong start_group_nb = dtus_per_block * nak.block_nb;

    /* end_group_nb corresponds to last group number of NAK-bitmap plus one */
    sb_ulong end_group_nb = min(nb_groups, dtus_per_block * (nak.block_nb + 1));

    /* number of valid bits in the incoming nak-bitmap */
    sb_ulong n = end_group_nb - start_group_nb;

    /* CALCULATE NUMBER OF STATUS BYTES IN PDU (8 DTUS STATUS PER BYTE). */
    const sb_ulong nak_bytes = (n+7) / 8;

    /* GET STARTING INDEX INTO "NAKS" ARRAY FOR THIS BLOCK */
    const sb_ulong nak_index = start_group_nb / 8;

    sb_uint8* bit8_p = nak_bitmap;
    sb_uint8* NakArray = naks + nak_index;

    sb_ulong bit_count = 0;

    /* IF THIS NACK PDU IS FROM A PREVIOUS PASS, IGNORE THE STATUS BITS FOR DTUs */
    /* THAT WE'VE JUST RETRANSMITTED IN THE CURRENT PASS.                        */
    if (nak.pass_nb < currentPass ) {
        sb_uint8* RetArray = retx + nak_index;
        for (sb_ulong i = 0; i < nak_bytes; i++) {
            if (*bit8_p) {
                *bit8_p &= ~*RetArray;          /* save only new bits              */
                
                *NakArray |= *bit8_p;           /* OR in new NACK status in MCB    */
                bit_count += Codeword::bit_count(*bit8_p);  /* count the bits in this NAK byte */
            }
            
            NakArray++;
            RetArray++;
            bit8_p++;
        }
    }
    else {
        assert(nak.pass_nb == currentPass);
        for (sb_ulong i = 0; i < nak_bytes; i++) {
            if (*bit8_p) {
                *NakArray |= *bit8_p;           /* OR in new NACK status in MCB    */
                bit_count += Codeword::bit_count(*bit8_p);  /* count the bits in this NAK byte */
            }
            
            NakArray++;
            bit8_p++;
        }
    }
    assert(bit_count == nak.nak_count);

    nakCount_++;    // increment total number of received nak-packets (status resp. packets)
}



sb_int MFTPSndAgent::user_file_init(sb_ulong filesize, sb_ulong dtusize,
                                    sb_ulong dtusPerBlock, sb_ulong dtusPerGroup,
                                    sb_ulong readAheadBufsize)
{
    sb_ulong ltmp = 0;
    sb_uchar last_status_byte;
    sb_ulong offset, max_nak_bytes;
    sb_ulong file_blocks = 0;

    read_ahead_bufsize = readAheadBufsize;

    /* STORE DTU SIZE IN TX STATUS */
    //    nmstats.MDUSize = dtu_size;

    /* STORE TOTAL BLOCKS IN TX STATUS */
    file_blocks = (FileDGrams + dtus_per_block - 1) / dtus_per_block;

    /* INITIALIZE CODEWORD PATTERN */
    nmstats.iterator.setSourceWordLen(dtus_per_group);
    nmstats.CwPat = nmstats.iterator.getNextCwPat();

    /* CALCULATE REQUIRED NUMBER OF BYTES FOR NAK/RETX BITMAPS */
    ltmp = dtus_per_block * file_blocks / 8;

    nak_bytes = ltmp;

    /* ALLOCATE NAK BITMAP INIT'D TO ALL ACKS FOR FILE RESTART */
    naks = (sb_uchar*) calloc(1, ltmp);

    /* ALLOCATE RETRANSMIT BITMAP INIT'D TO NONE RETRANSMITTED */
    retx = (sb_uchar*) calloc(1, ltmp);

    /* REPORT ERROR ON NAK BITMAP ALLOCATION */
    if (naks == NULL)
        {
            perror("Agent/MFTPSnd: couldn't allocate memory for nack bitmap\n");
            exit(-1);
        }

    /* REPORT ERROR ON NAK BITMAP ALLOCATION */
    if (retx == NULL) {
        perror("Agent/MFTPSnd: couldn't allocate memory for retransmission bitmap\n");
        exit(-1);
    }

    memset(naks, -1, ltmp);

    /* NOTE WELL:
     * THE FOLLOWING CODE IS IDENTICAL TO CODE THAT
     * ALLOCATES THE "NAK" ARRAY FOR THE CLIENT ( ce_alloc()).
     * THIS CODE BASICALLY SETS ALL INSIGNIFICANT BITS OF
     * THE NAK ARRAY TO "ACK" STATUS SO THAT THE SERVER WILL
     * NEVER INCLUDE THEM IN THE RE-TRANSMISSION STREAM.
     */

    /* IF PARTIAL LAST BLOCK */
    if( FileDGrams % dtus_per_block ) {
        /* GET MAXIMUM STATUS BYTES IN A NAK PDU */
        max_nak_bytes = dtus_per_block / 8;

        /* GET NUMBER OF BLOCKS */
        offset = FileDGrams / dtus_per_block;

        /* GET "NAKS" INDEX TO LAST BLOCK (ZERO-BASED) */
        offset *= max_nak_bytes;

        /* GET "NAKS" INDEX TO LAST BYTE OF LAST BLOCK */
        offset += ((FileDGrams % dtus_per_block) / 8);

        /* IF DTUS IN LAST BLOCK NOT MULTIPLE OF 8 */
        if( (FileDGrams % dtus_per_block) % 8 )
            /* INCREMENT FOR PARTIAL LAST STATUS BYTE */
            offset++;

        /* ZERO-BASED */
        offset--;

        /* PRESET ALL BITS OF LAST BYTE TO NAK */
        last_status_byte = 0xFF;

        /* IF NUMBER DTUS NOT MULTIPLE OF 8.. */
        if( FileDGrams % 8 ) {
            /* ..FIND NUMBER OF INSIGNIFICANT STATUS BITS TO SET TO ACK */
            sb_int padding = 8 - ((sb_int)(FileDGrams % 8));

            /* WHILE MORE INSIGNIFICANT BITS.. */
            while( padding ) {
                /* SET THE ASSOCIATED BIT TO ACK */
                last_status_byte &= ~( 1 << (8 - padding) );

                /* DECREMENT NUMBER OF BITS REMAINING*/
                padding--;
            }
        }

        /* STORE LAST BYTE WITH INSIGNIFICANT ACK BITS */
        last_nak      = offset;
        last_nak_mask = last_status_byte;

        ((sb_char*)naks)[offset] = last_nak_mask;

        /* STEP TO NEXT BYTE BEYOND LAST SIGNIFICANT BYTE */
        offset++;

        /* STORE INSIGNIFICANT ACKS IN REST OF PARTIAL LAST BLOCK */
        for( ; offset < nak_bytes; offset++ )
            ((sb_char*)naks)[offset] = 0;
    }

    fseek_offset = 0;
    return(0);
}




/* reads as many groups into the read-ahead-buffer as there is space, starting with */
/* group nmstats->CurrentGroup. The groups that were not not NACK'd by anyone will */
/* be skipped (and the corresponding areas in the read-ahead-buffer will be skipped */
/* as well. */
sb_int MFTPSndAgent::fill_read_ahead_buf()
{
    sb_uint dtu_pos;        // loops over [0..dtus_per_group)
    sb_ulong seek_offset;   // where to position the head for disk seeks
    sb_ulong buf_pos = 0;   // position where data is written (into main memory) when
                            // read from disk, relative to the start of read_ahead_buf
    //sb_int status = 0;
    CW_PATTERN_t cw_pat_tmp = nmstats.CwPat;
    sb_ulong i;
    sb_ulong len;

    /* switch to next group that must be read: */
    nmstats.MinGroupNbInBuf = nmstats.CurrentGroup;
    nmstats.NbGroupsInBuf = min(read_ahead_bufsize /
                                 (Codeword::bit_count(nmstats.CwPat)*dtu_size),
                                 nb_groups - nmstats.MinGroupNbInBuf);
    while(cw_pat_tmp) {
        dtu_pos = Codeword::minbit(cw_pat_tmp);
        assert(0 <= dtu_pos && dtu_pos < dtus_per_group);
        assert(nmstats.MinGroupNbInBuf + nmstats.NbGroupsInBuf <= nb_groups);

        cw_pat_tmp &= ~((CW_PATTERN_t) 1 << dtu_pos);  /* clear bit at position "dtu_pos" */

        for(i = nmstats.MinGroupNbInBuf;
            i < nmstats.MinGroupNbInBuf + nmstats.NbGroupsInBuf; ++i) {

            /* continue with for-loop if group i was not NACKed by anyone */
            if(IS_BIT_CLEARED(naks, i)) {
                buf_pos += dtu_size;
                continue;
            }

            /* Note: there is never data accessed "outside" the file as the while-loop */
            /* is left as soon as the last (possibly partial) DTU has been read. */
            seek_offset = (dtu_pos * nb_groups + i) * dtu_size;
            if(seek_offset >= FileSize) {
                /* we can get there if the last group(s) have fewer than */
                /* dtus_per_group packets. If we get here, we are ready. */
                return 0;                 /* OK */
            }

            if (fseek_offset != seek_offset) {
                /* do the fseek here (omitted) */
                seekCount_++;
            }
            fseek_offset = seek_offset;

            /* determine number of bytes to read */
            len = min(dtu_size, FileSize - seek_offset);

            /* read len bytes from file here (omitted) */

            fseek_offset = seek_offset + len;
            buf_pos += len;
            if(len < dtu_size) {
                /* we get here if the last dtu is smaller than dtu_size and if */
                /* we have just read that last dtu */

                assert(fseek_offset == FileSize); /* we must be at EOF */

                /* clear rest of read-ahead-buffer here (omitted) */
                buf_pos = Codeword::bit_count(nmstats.CwPat) * nmstats.NbGroupsInBuf * dtu_size;
                return 0;                 /* that's it, no more packets to process */
             }
            assert(len == dtu_size);
            assert(buf_pos <= Codeword::bit_count(nmstats.CwPat) * nmstats.NbGroupsInBuf * dtu_size);
        } /* for */
    } /* while */
    /* we get here only if no group was read with less than dtus_per_group packets and */
    /* the if not the last packet was read (in case it is too short) */
    assert(buf_pos == Codeword::bit_count(nmstats.CwPat) * nmstats.NbGroupsInBuf * dtu_size);
    return 0;  /* OK */
}


// send_data() sends next packet */
sb_int MFTPSndAgent::send_data()
{
    sb_int result;
    sb_ushort length = 0;
    Packet* p = Agent::allocpkt();
    hdr_mftp* hdr = (hdr_mftp*) p->access(off_mftp_);
    CW_PATTERN_t mask = 0;
    Tcl& tcl = Tcl::instance();

    assert(0 <= nmstats.CurrentGroup && nmstats.CurrentGroup < nb_groups);
    assert(nmstats.NbGroupsInBuf >= 0);
    assert(0 <= nmstats.MinGroupNbInBuf &&
           nmstats.MinGroupNbInBuf + nmstats.NbGroupsInBuf <= nb_groups);

    /* now comes NACK processing */
    /* UNTIL LAST GROUP OF FILE CHECKED OR A NAK STATUS IS DETECTED */
    while(nmstats.CurrentGroup < nb_groups &&
          IS_BIT_CLEARED(naks, nmstats.CurrentGroup)) {
        /* SKIP TO NEXT BIT OF THE NAK BITMAP */
        nmstats.CurrentGroup++;
    }

    /* do not transmit packet if */
    /* 1.) CurrentGroup has reached the total number of groups ("end of pass") or */
    /* 2.) CwPat has only bits set that refer to some packets that are cut off in */
    /*     the current group (for example, if CurrentGroup has only 5 packets */
    /*     with (nb_groups:=8) and if CwPat == 64+32) */
    if(nmstats.CurrentGroup != nb_groups &&
       ((mask = (1 << get_dtus_per_group(nmstats.CurrentGroup)) - 1) & nmstats.CwPat)) {

        assert(nmstats.CurrentGroup < nb_groups);

        /* see if the read-ahead-buffer is exhausted so that we must load new data */
        /* from the file. Only those groups are read that have a corresponding NACK-bit */
        /* set, i.e. that were requested for retransmission */
        assert(nmstats.MinGroupNbInBuf <= nmstats.CurrentGroup);
        if(nmstats.CurrentGroup >= nmstats.MinGroupNbInBuf + nmstats.NbGroupsInBuf) {
            if((result = fill_read_ahead_buf())) {
                return TCL_ERROR;
            }
        }
        assert(nmstats.MinGroupNbInBuf <= nmstats.CurrentGroup &&
               nmstats.CurrentGroup < nmstats. MinGroupNbInBuf + nmstats.NbGroupsInBuf);

        /* GET LENGTH OF DTU PAYLOAD */
        length = (sb_ushort) dtu_size;

        /* produce and encoded packet here (omitted) */

        /* GENERATE THE HEADER */
        hdr->type = hdr_mftp::PDU_DATA_TRANSFER;
        hdr->spec.data.pass_nb  = nmstats.CurrentPass;
        hdr->spec.data.group_nb = nmstats.CurrentGroup;
        hdr->spec.data.cw_pat   = nmstats.CwPat & mask;

        tcl.evalf("%s send-notify %lu %lu %lu", name_,
                  (unsigned long) nmstats.CurrentPass,
                  (unsigned long) nmstats.CurrentGroup,
                  (unsigned long) nmstats.CwPat & mask);

        //tcl.evalf("%s send-notify %lu %lu %llu", name_,
        //          (unsigned long) nmstats.CurrentPass,
        //          (unsigned long) nmstats.CurrentGroup,
        //          (unsigned long long) nmstats.CwPat & mask);

        /* RESET THE DTU STATUS BIT IN THE NACK BITMAP */
        RESET_BIT(naks, nmstats.CurrentGroup);

        /* SET THE DTU STATUS BIT IN THE RETRANSMISSION BITMAP */
        SET_BIT(retx, nmstats.CurrentGroup);

        /* transmit packet */
        hdr_cmn* ch = (hdr_cmn*) p->access(off_cmn_);
        ch->size() = sizeof(hdr_mftp) + length;
        target_->recv(p);

        nmstats.CurrentGroup++;
    } /* if */

    // if last group of the file:
    if(nmstats.CurrentGroup == nb_groups || !(nmstats.CwPat & mask)) {
        /* end of pass */
        do {
            /* get next codeword for new pass */
            nmstats.CwPat = nmstats.iterator.getNextCwPat();
        } while(!(nmstats.CwPat & ((1 << get_dtus_per_group(0)) - 1)));

        /* start over: */
        nmstats.MinGroupNbInBuf = 0;
        nmstats.NbGroupsInBuf = 0;
        nmstats.CurrentGroup = 0;

        /* RESET RETX BITMAP FOR QUALIFICATION OF LATENT NAKS */
        RESET_ALL_BITS(retx, nb_groups);

        /* the first dtus_per_group rounds must be "fully" transmitted! */
        if(nmstats.CurrentPass < dtus_per_group - 1) {
            SET_ALL_BITS(naks, nb_groups);
        }

        tcl.evalf("%s round-finished %lu %lu", name_,
                  (unsigned long) nmstats.CurrentPass, (unsigned long) nb_blocks());


        nmstats.CurrentPass++;

        /* return end of pass to the caller: */
        tcl.result("-1");
        return TCL_OK;
    }
    tcl.result("0");
    return TCL_OK;
}


