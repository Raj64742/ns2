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
 * File: mftp_rcv.cc
 * Last change: Jan. 13, 1998
 *
 * This software may freely be used only for non-commercial purposes
 */

/*
 *
 *   This file is only relevant for the MFTP-client.
 *
 *   PUBLIC FUNCTIONS:
 *
 *   recv                             - process received packet (overloaded function)
 *
 *
 *   PROTECTED FUNCTIONS:
 *
 *   static sb_int findStoreLocation  - finds "nearest" free slot to store a received
 *                                      frame on disk with resp. to current seek-position
 *
 *
 *
 *******************************************************************************/


#include <assert.h>

#include "Tcl.h"
#include "mftp_rcv.h"

#include "sb.h"            // due to declaration of sb_ulong, etc.
#include "ip.h"            // due to declaration of hdr_ip

#define min(a, b)       ((a) < (b) ? (a) : (b))

static class MFTPRcvAgentClass : public TclClass {
public:
        MFTPRcvAgentClass() : TclClass("Agent/MFTP/Rcv") {}
        TclObject* create(int, const char*const*) {
		return (new MFTPRcvAgent());
	}
} class_mftprcv_agent;


MFTPRcvAgent::MFTPRcvAgent() 
        : MFTPAgent(),
          cw_matrixline_buf(NULL),
          last_dtu_buf(NULL)
{
    bind("reply_", (int*)&reply_);
}

// inspect a Tcl command (overloaded function):
int MFTPRcvAgent::command(int argc, const char*const* argv)
{
    Tcl& tcl = Tcl::instance();
    if(strcmp(argv[1], "send") == 0) {
        if(strcmp(argv[2], "nak") == 0) {
            unsigned long pass_nb, block_lo, block_hi, group_lo, group_hi;
            int nb_scanned = 0;

            nb_scanned += sscanf(argv[3], "%ul", &pass_nb);
            nb_scanned += sscanf(argv[4], "%ul", &block_lo);
            nb_scanned += sscanf(argv[5], "%ul", &block_hi);
            nb_scanned += sscanf(argv[6], "%ul", &group_lo);
            nb_scanned += sscanf(argv[7], "%ul", &group_hi);
            assert(nb_scanned == 5);
            while(block_lo <= block_hi) {
                send_nak(pass_nb, block_lo); 
                block_lo++;
            }
            return TCL_OK;
        }
    } else if (strcmp(argv[1], "start") == 0) {
        if(dtusPerBlock_ % 8 != 0) {
            tcl.resultf("%s: dtusPerBlock_ must be a multiple of 8", name_);
            return TCL_ERROR;
        }
        init();
        return TCL_OK;
    }
    return Agent::command(argc, argv);
}

// process reception of a packet
void MFTPRcvAgent::recv(Packet* p, Handler* h)
{
    hdr_ip* ih = (hdr_ip*) p->access(off_ip_);
    hdr_mftp* mh = (hdr_mftp*) p->access(off_mftp_);

    if(ih->dst() == 0) {
        // Packet from local agent
        fprintf(stderr, "%s: send not allowed with Agent/MFTP/Rcv\n", name_);
        exit(0);
    } else {
        switch(mh->type) {
        case hdr_mftp::PDU_DATA_TRANSFER:
            recv_data(mh->spec.data);
            break;
        case hdr_mftp::PDU_STATUS_REQUEST:
            recv_status_req(mh->spec.statReq);
            break;
        case hdr_mftp::PDU_NAK:
            /* as we are a member of the group as well, we receive all data */
            /* we have sent. */
            break;
        default:
            assert(false); // received unknown packet type
        }
        Packet::free(p);
    }
}

// Destructor:
MFTPRcvAgent::~MFTPRcvAgent()
{
    // Note: delete on a NULL-pointer has no effect
    delete [] cw_matrixline_buf;
    delete [] last_dtu_buf;
}


sb_void MFTPRcvAgent::init()
{
    MFTPAgent::init();

    // allocate cw_matrix_line_buf
    assert(cw_matrixline_buf == NULL);
    cw_matrixline_buf = new CW_MATRIXLINE_t[FileDGrams];
    assert(cw_matrixline_buf != NULL);  // or else no memory is left!
    // should return an error instead of terminating the program

    /* reset array: */
    cw_matrixlines_reset();

    // allocate last_dtu_buf
    assert(last_dtu_buf == NULL);
    last_dtu_buf = new sb_uchar[dtu_size];
    assert(last_dtu_buf != NULL);  // or else no memory is left!
    // should return an error instead of terminating the program
    memset(last_dtu_buf, 0, dtu_size);
}


/* process a received status request packet */
sb_void MFTPRcvAgent::recv_status_req(hdr_mftp::Spec::StatReq& statreq)
{
    Tcl& tcl = Tcl::instance();

    /* read the PDU_STATUS_REQUEST-specific fields: */
    tcl.evalf("%s recv status-req %lu %lu %lu %lu %lu %lf", name_,
              (unsigned long) statreq.pass_nb,
              (unsigned long) statreq.block_lo,
              (unsigned long) statreq.block_hi,
              (unsigned long) statreq.group_lo,
              (unsigned long) statreq.group_hi,
              (double) statreq.RspBackoffWindow);
}

// function analog to ce_send_stat (ce_tx.c)
sb_void MFTPRcvAgent::send_nak(sb_ulong pass_nb, sb_ulong block_nb)
{
    assert(FileDGrams > 0);
    assert(0 <= block_nb && block_nb < nb_blocks());

    /* start_group_nb corresponds to first bit of NAK-bitmap: */
    sb_ulong start_group_nb = dtus_per_block * block_nb;

    /* end_group_nb corresponds to last group number of NAK-bitmap plus one */
    sb_ulong end_group_nb = min(nb_groups, dtus_per_block * (block_nb + 1));

    assert(start_group_nb < end_group_nb);
    /* number of valid bits in the outgoing nak-bitmap */
    sb_ulong n = end_group_nb - start_group_nb;

    /* CALCULATE NUMBER OF STATUS BYTES IN PDU (8 DTUS STATUS PER BYTE). */
    const sb_ulong nak_bytes = (n+7) / 8;

    sb_ulong bit_count = 0;

    // allocate (get) new packet and dynamically allocate extra space for nak-bitmap:
    Packet* p = Agent::allocpkt((n+7) / 8);

    sb_uint8* byte_p = (sb_uint8*) p->accessdata();

    /* clear NAK-bitmap first: */
    memset(byte_p, 0, nak_bytes);
    
    /* LOOP OVER ALL GROUPS IN RANGE OF NAK AND SET NAK-BIT FOR THOSE THAT ARE NOT STILL FULL */
    /* (CH: complete revision) */
    for(sb_ulong group_nb = start_group_nb, bit = 1 << (start_group_nb % 8);
        group_nb < end_group_nb; ++group_nb) {
        if(!group_full(group_nb)) {
            *byte_p |= bit;
            bit_count++;
        }
        if(bit == 128) {
            bit = 1;
            byte_p++;
        } else {
            bit <<= 1;
        }
    }

    if(bit_count > 0) {
        hdr_ip* iph = (hdr_ip*)p->access(off_ip_);
        hdr_mftp* hdr = (hdr_mftp*) p->access(off_mftp_);
        hdr_cmn* ch = (hdr_cmn*) p->access(off_cmn_);

        /* now generate the header */
	iph->dst() = reply_;    // overwrite settings from Agent::allocpkt()
        ch->size() = sizeof(hdr_mftp);

        hdr->type = hdr_mftp::PDU_NAK;
        hdr->spec.nak.pass_nb   = pass_nb;
        hdr->spec.nak.block_nb  = block_nb;
        hdr->spec.nak.nak_count = bit_count;

        /* transmit frame */
        target_->recv(p);
    }
    else {
        Packet::free(p);    // do not transmit NAK-packet if it consists of 0 NAK-bits !!
                            // HACK: @ requires optimation still!
    }
}

// process_frame_no: decides if a received frame is "useful" and
// stores meta-information for proper decoding
sb_int MFTPRcvAgent::process_frame_no(CW_PATTERN_t cw_pat,
                                      sb_ulong group_nb, sb_ulong dtu_nb)
{
    CW_PATTERN_t bit;
    CW_MATRIXLINE_t new_row;

    sb_ulong j;         /* j iterates over the dtus of group "group_nb" */
    sb_ulong finish = get_dtus_per_group(group_nb);
                        /* finish counts the number of dtus in group "group_nb" */

    new_row.left = cw_pat;

    for(j = 0; j < finish; j++) {
        /* j iterates over the dtus of group "group_nb" */
        CW_PATTERN_t line_pat =
            cw_matrixline_buf[j * nb_groups + group_nb].left;
        if(line_pat) {
            bit = new_row.left & ((CW_PATTERN_t) 1 << Codeword::minbit(line_pat));
            if(bit) {
                new_row.left ^= line_pat;
            }
        }
    } /* for j */
    if(new_row.left) { /* linearly independent? */
        bit = (CW_PATTERN_t) 1 << Codeword::minbit(new_row.left);
        for(j = 0; j < finish; j++) {
            /* j iterates over the dtus of group "group_nb" */
            if(bit & cw_matrixline_buf[j * nb_groups + group_nb].left) {
                cw_matrixline_buf[j * nb_groups + group_nb].left ^= new_row.left;
            }
        }
        /* register pattern of codeword the received frame is composed of (possibly altered) */
        /* must be done at last for that this line gets not erased by XORing with itself */
        /* in previous loop */
        cw_matrixline_buf[dtu_nb * nb_groups + group_nb].left = new_row.left;
        return 1; /* frame was a "new" frame (i.e. is linearly independent from the other ones */
        /* received so far) */
    }
    else {
        /* linearly dependent codeword-pattern received, i.e. useless */
        return 0;
    }
}


sb_int MFTPRcvAgent::findStoreLocation(sb_ulong group_nb, sb_ulong seek_offset, sb_ulong* dtu_nb)
{
    sb_ulong start_dtu_nb;

    assert(0 <= group_nb && group_nb < nb_groups);
    assert(0 <= seek_offset && seek_offset <=
           dtu_size * (FileDGrams - 1) + end_dtu_size);
    assert(seek_offset % dtu_size == 0 ||
           seek_offset == FileSize);

    if(seek_offset == FileSize) {
        *dtu_nb = group_nb;  /* start over */
    } else {
        sb_ulong curr_dtu_nb = nmstats.FseekOffset / dtu_size;

        /* pay attention to "unsigned" when substracting */
        *dtu_nb = curr_dtu_nb - curr_dtu_nb % nb_groups;
        *dtu_nb += group_nb;

        /* check if seeking backwards. If yes, increment dtu_nb by nb_groups to */
        /* always seeks forwards (unless end of file is reached):               */
        if(*dtu_nb < curr_dtu_nb) {
            *dtu_nb += nb_groups;
        }
    }
    if(*dtu_nb >= FileDGrams) {
        /* this might happen if some groups have less packets than */
        /* dtus_per_group: */
        *dtu_nb = group_nb;    /* start over from beginning */
    }
    start_dtu_nb = *dtu_nb;
    assert(start_dtu_nb < FileDGrams);

    do {
        if(! cw_matrixline_buf[*dtu_nb].left) {
            return 1;
        }
        *dtu_nb += nb_groups;
        if(*dtu_nb >= FileDGrams) {
            *dtu_nb = group_nb;    /* start over from beginning */
        }
    } while(*dtu_nb != start_dtu_nb);
    return 0;    /* return: group "group_nb" is already full */
}


/* initializes all matrix-lines to zero, i.e. no (coded) packets at all are received so far: */
sb_void MFTPRcvAgent::cw_matrixlines_reset()
{
    assert(FileDGrams > 0);
    memset(cw_matrixline_buf, 0, sizeof(CW_MATRIXLINE_t) * FileDGrams);
}


/* returns 1 if group "group_nb" is full, 0 if there is at least one frame missing */
sb_int MFTPRcvAgent::group_full(sb_ulong group_nb)
{
    sb_ulong nb_dtus = get_dtus_per_group(group_nb);
    sb_ulong i;

    assert(0 <= group_nb && group_nb < nb_groups);

    for(i = 0; i < nb_dtus &&
            cw_matrixline_buf[i * nb_groups + group_nb].left; ++i)
        ;
    return (i == nb_dtus); /* if left loop before nb_dtus was reached, then */
    /* there is some line in the matrix that is all "0", i.e. a frame is still */
    /* missing. */
}

// recv_data: process received data packet;
// takes (received) coded packets and processes them.
sb_int MFTPRcvAgent::recv_data(hdr_mftp::Spec::Data& data)
{
    Tcl& tcl = Tcl::instance();
    sb_ulong seek_offset;
    sb_ulong dtu_nb; /* position (in terms of datagram number) where incoming */
    /* frame is stored in file */
    
    sb_uchar last_pdu[dtu_size];
    
    /* read the PDU_DATA_TRANSFER-specific fields: */
    nmstats.CurrentPass  = data.pass_nb;
    nmstats.CurrentGroup = data.group_nb;
    nmstats.CwPat = data.cw_pat;

    /* validate fields: */
    /* (actually, assert should be replaced by just ignoring the packet in case */
    /* the parameters are invalid, as some corrupt packet might reach the receiver */
    /* in real world, i.e. this would not be a bug in the software!) */
    assert(0 <= nmstats.CurrentPass);
    assert(0 <= nmstats.CurrentGroup && nmstats.CurrentGroup < nb_groups);
    assert(Codeword::is_valid(nmstats.CwPat, dtus_per_group));


    if(findStoreLocation(nmstats.CurrentGroup, nmstats.FseekOffset, &dtu_nb)) {
        /* arriving frame belongs to a not already full group: */
        assert(dtu_nb % nb_groups == nmstats.CurrentGroup);
        assert(0 <= dtu_nb && dtu_nb < FileDGrams);

        if(process_frame_no(nmstats.CwPat,
                            nmstats.CurrentGroup,
                            dtu_nb / nb_groups)) {
            cw_matrixline_buf[dtu_nb].right = nmstats.CwPat;

            /* arriving frame is useful (i.e. linearly independent from the others */
            /* of the group, thus store frame on disk: */

            tcl.evalf("%s recv useful %lu %lu %lu", name_,
                      (unsigned long) nmstats.CurrentPass,
                      (unsigned long) nmstats.CurrentGroup,
                      (unsigned long) nmstats.CwPat);
                
            seek_offset = dtu_nb * dtu_size;
            if(dtu_nb == FileDGrams - 1) {
                /* the last dtu of the file might not fit into the file, as with */
                /* erasure correction, the dtu size must always be the same,     */
                /* i.e. dtu_size. So we don't write the packet on disk.    */
                /* Rather, we store it in a special place in main memory.        */
                // memcpy(last_dtu_buf, data_p, (sb_int) parm->length);
            } else {
                /* prepare to write the new packet to the file system: */
                if(nmstats.FseekOffset != seek_offset) {
                    // seek to file-position seek_offset (omitted)
                    nmstats.FseekOffset = seek_offset;
                    seekCount_++;
                }
                // write data to file here (omitted)
                nmstats.FseekOffset += dtu_size;
            } /* else */
            /* INCREMENT NUMBER OF GOOD DTUS RECEIVED */
            nmstats.FileDGramsReceived++;
                
            /* RESET BIT IN NAK ARRAY TO INDICATE WE'VE RECEIVED THE DTU */
            /* CH: NEEDS REVISION */
            /* ((sb_uchar*)(naks))[offset] &= ~(1 << bit); */

            /*
             * IF ALL PACKETS HAVE NOW BEEN RECEIVED, THEN CLOSE THE DATA FILE,
             * FREE RESOURCES. SEND A DONE MESSAGE AND CHANGE STATE TO DONE PENDING.
             */

            if (nmstats.FileDGramsReceived == FileDGrams) {
                // decode file here (omitted)
                tcl.evalf("%s done-notify %lu %lu %lu", name_,
                      (unsigned long) nmstats.CurrentPass,
                      (unsigned long) nmstats.CurrentGroup,
                      (unsigned long) nmstats.CwPat);

                return(0);   // we are ready!
            }
        } /* if(process_frame_no...) */
        else {
            tcl.evalf("%s recv dependent %lu %lu %lu", name_,
                      (unsigned long) nmstats.CurrentPass,
                      (unsigned long) nmstats.CurrentGroup,
                      (unsigned long) nmstats.CwPat);
        }                
    } /* if(findStoreLocation...) */
    else {
        /* we received a frame that belongs to an already full group */
        tcl.evalf("%s recv group-full %lu %lu %lu", name_,
                  (unsigned long) nmstats.CurrentPass,
                  (unsigned long) nmstats.CurrentGroup,
                  (unsigned long) nmstats.CwPat);
    }
    return(0);
}

  
