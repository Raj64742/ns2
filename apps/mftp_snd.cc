/*
 * (c) 1997-98 StarBurst Communications Inc.
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
 * Last change: Dec 07, 1998
 *
 * This software may freely be used only for non-commercial purposes
 */


// This file contains functionality specific to the MFTP sender.

#include <stdlib.h>     // strtoul, etc.
#include <assert.h>

#include <stdio.h>

#include "config.h"
#include "tclcl.h"
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
      naks(0),
      retx(0),
      fseek_offset(0),
      read_ahead_bufsize(0),
      CurrentPass(0),
      CurrentGroup(0),
      CwPat(0),
      MinGroupNbInBuf(0),
      NbGroupsInBuf(0)
{
    bind("readAheadBufsize_", &readAheadBufsize_);
    bind_time("txStatusDelay_", &txStatusDelay_);
    bind("nakCount_", &nakCount_);
}

MFTPSndAgent::~MFTPSndAgent()
{
    delete [] naks;  // NOTE: delete on NULL pointer has no effect
    delete [] retx;
}

int MFTPSndAgent::command(int argc, const char*const* argv)
{
    Tcl& tcl = Tcl::instance();

    if(strcmp(argv[1], "send") == 0) {
        if(strcmp(argv[2], "data") == 0) {
            return send_data();
        } else if(strcmp(argv[2], "statreq") == 0) {
            unsigned long pass_nb, block_lo, block_hi;
            double rsp_backoff_window=2343.2343;
            int nb_scanned = 0;
            if(argc == 7) {
                nb_scanned += sscanf(argv[3], "%lu", &pass_nb);
                nb_scanned += sscanf(argv[4], "%lu", &block_lo);
                nb_scanned += sscanf(argv[5], "%lu", &block_hi);
                nb_scanned += sscanf(argv[6], "%lf", &rsp_backoff_window);
            }
            if(nb_scanned != 4) {
                tcl.resultf("%s: wrong number of parameters for \"send statreq\"", name_);
                return TCL_ERROR;
            }
            send_status_request(pass_nb, block_lo, block_hi, rsp_backoff_window); 
            return TCL_OK;
        }
    }
    if(strcmp(argv[1], "start") == 0) {
        if(MFTPAgent::init() == TCL_ERROR) {
            return TCL_ERROR;
        };
        init_user_file((unsigned long) readAheadBufsize_);
        return TCL_OK;
    }
    return Agent::command(argc, argv);
}

void MFTPSndAgent::recv(Packet* p, Handler* h)
{
    hdr_ip* ih = (hdr_ip*) p->access(off_ip_);
    hdr_mftp* mh = (hdr_mftp*) p->access(off_mftp_);

    if(ih->daddr() == 0) {
        assert(false);        // Packet from local agent.
    } else {
        switch(mh->type) {
        case hdr_mftp::PDU_DATA_TRANSFER:
        case hdr_mftp::PDU_STATUS_REQUEST:
            // as the sender is a member of the multicast group as well,
            // it receives all data it has sent. So just ignore it.
            break;
        case hdr_mftp::PDU_NAK:
            process_nak(mh->spec.nak, p->accessdata(), CurrentPass-1); // -1 because we have
            // incremented the pass-number already in send_data.
            break;
        default:
            assert(false);  // unknown packet type (also possible: just ignore packet rather than exit)
        }
        Packet::free(p);
    }
}

void MFTPSndAgent::send_status_request(unsigned long pass_nb,
                                       unsigned long block_lo,
                                       unsigned long block_hi,
                                       double rsp_backoff_window)
{
    Packet* p = Agent::allocpkt();
    hdr_mftp* hdr = (hdr_mftp*) p->access(off_mftp_);

    assert(FileDGrams > 0);                    // we need this requirement here

    // initialize the header of the status request packet:
    hdr->type = hdr_mftp::PDU_STATUS_REQUEST;
    hdr->spec.statReq.pass_nb  = pass_nb;
    hdr->spec.statReq.block_lo = block_lo;
    hdr->spec.statReq.block_hi = block_hi;
    hdr->spec.statReq.RspBackoffWindow = rsp_backoff_window;

    // transmit packet
    hdr_cmn* ch = (hdr_cmn*) p->access(off_cmn_);
    ch->size() = sizeof(hdr_mftp);
    target_->recv(p);
}


// process incoming nak:
void MFTPSndAgent::process_nak(hdr_mftp::Spec::Nak& nak,
                                  unsigned char* nak_bitmap,
                                  unsigned long currentPass)
{
    assert(1 <= nak.nak_count && nak.nak_count <= nb_groups); // or else some receiver is fooling us.
    assert(nak.pass_nb <= currentPass);  // pass greater than requested? => a receiver is fooling us.

    Tcl& tcl = Tcl::instance();
    tcl.evalf("%s recv nak %lu %lu %lu", name_,
              (unsigned long) nak.pass_nb,
              (unsigned long) nak.block_nb,
              (unsigned long) nak.nak_count);

    assert(dtus_per_block % 8 == 0);         // This property is required for the following

    // start_group_nb corresponds to first bit of NAK-bitmap:
    const unsigned long start_group_nb = dtus_per_block * nak.block_nb;

    // end_group_nb corresponds to last group number of NAK-bitmap plus one
    const unsigned long end_group_nb = min(nb_groups, dtus_per_block * (nak.block_nb + 1));

    // get starting index into naks-array for this block
    const unsigned long nak_index = start_group_nb / 8;

    // number of status bytes in pdu
    const unsigned long nak_bytes = (end_group_nb - start_group_nb + 7) / 8;

    // pointer to location in array at which the received nak bitmap must be
    // or'd to the sender-bitmap (the bitmap in which the sender collects the naks)
    unsigned char* nak_array = naks + nak_index;

    // if this nak pdu is from a previous pass (i.e. a delayed nak), ignore the status
    // bits for dtu's that we've just retransmitted in the current pass:
    if(nak.pass_nb < currentPass) {
        unsigned char* retx_array = retx + nak_index;
        for(unsigned long i = 0; i < nak_bytes; i++) {
            if(*nak_bitmap) {
                // "AND out" bits for already transmitted packets and
                // "OR in" the result into newly constructed NAK bitmap
                *nak_array |= (*nak_bitmap & (~*retx_array));
            }            
            nak_array++;
            retx_array++;
            nak_bitmap++;
        }
    }
    else {
        assert(nak.pass_nb == currentPass); // this nak belongs to the current pass
        for(unsigned long i = 0; i < nak_bytes; i++) {
            if(*nak_bitmap) {
                // "OR in" NAK byte into newly constructed NAK bitmap
                *nak_array |= *nak_bitmap;
            }
            nak_array++;
            nak_bitmap++;
        }
    }
    nakCount_++;    // increment total number of received nak-packets
}



void MFTPSndAgent::init_user_file(unsigned long readAheadBufsize)
{
    read_ahead_bufsize = readAheadBufsize;
    fseek_offset = 0;

    // initialize codeword pattern
    iterator.setSourceWordLen(dtus_per_group);
    CwPat = iterator.getNextCwPat();
    // free arrays from a possible previous transmission (no effect on NULL pointers)
    delete [] naks;
    delete [] retx;

    // allocate naks bitmap init'd to all nak'd:
    naks = new unsigned char[(nb_groups + 7) / 8];
    assert(naks != NULL);           // or else we ran out of memory
    SET_ALL_BITS(naks, nb_groups);

    // allocate retransmission bitmap init'd to none retransmitted:
    retx = new unsigned char[(nb_groups + 7) / 8];
    assert(retx != NULL);           // or else we ran out of memory
    RESET_ALL_BITS(retx, nb_groups);

    CurrentPass = CurrentGroup = MinGroupNbInBuf = NbGroupsInBuf = 0;
}




// reads as many groups into the read-ahead-buffer as there is space, starting with
// group CurrentGroup. The groups that were not not NACK'd by anyone will be
// skipped (and the corresponding areas in the read-ahead-buffer will be skipped
// as well).
void MFTPSndAgent::fill_read_ahead_buf()
{
    unsigned int dtu_pos;        // loops over [0..dtus_per_group)
    unsigned long seek_offset;   // where to position the head for disk seeks
    unsigned long buf_pos = 0;   // position where data is written (into main memory) when
                                 // read from disk, relative to the start of read_ahead_buf
    CW_PATTERN_t cw_pat_tmp = CwPat;
    unsigned long i;
    unsigned long len;

    // switch to next group that must be read:
    MinGroupNbInBuf = CurrentGroup;
    NbGroupsInBuf = min(read_ahead_bufsize / (bitcount(CwPat) * dtu_size),
                        nb_groups - MinGroupNbInBuf);
    while(cw_pat_tmp != 0) {
        dtu_pos = minbit(cw_pat_tmp);
        assert(0 <= dtu_pos && dtu_pos < dtus_per_group);
        assert(MinGroupNbInBuf + NbGroupsInBuf <= nb_groups);

        cw_pat_tmp &= ~((CW_PATTERN_t) 1 << dtu_pos);  // clear bit at position "dtu_pos"

        for(i = MinGroupNbInBuf;
            i < MinGroupNbInBuf + NbGroupsInBuf; ++i) {

            // continue with for-loop if group i was not NACKed by anyone
            if(IS_BIT_CLEARED(naks, i)) {
                buf_pos += dtu_size;
                continue;
            }

            // Note: there is never data accessed "outside" the file as the while-loop
            // is left as soon as the last (possibly partial) DTU has been read.
            seek_offset = (dtu_pos * nb_groups + i) * dtu_size;
            if(seek_offset >= FileSize) {
                // we can get there if the last group(s) have fewer than
                // dtus_per_group packets. If we get here, we are ready.
                return; // OK
            }

            if (fseek_offset != seek_offset) {
                // do the fseek here (omitted)
                seekCount_++;
                fseek_offset = seek_offset;
            }

            // determine number of bytes to read
            len = min(dtu_size, FileSize - fseek_offset);

            // read len bytes from file here (omitted)
            fseek_offset += len;

            buf_pos += len;
            if(len < dtu_size) {
                // we get here if the last dtu is smaller than dtu_size and if
                // we have just read that last dtu

                assert(fseek_offset == FileSize); // we must be at EOF

                // clear rest of read-ahead-buffer here (omitted)
                buf_pos = bitcount(CwPat) * NbGroupsInBuf * dtu_size;
                return;                  // that's it, no more packets to process
             }
            assert(len == dtu_size);
            assert(buf_pos <= bitcount(CwPat) * NbGroupsInBuf * dtu_size);
        } // for
    } // while
    // we get here only if no group was read with less than dtus_per_group packets and
    // the if not the last packet was read (in case it is too short)
    assert(buf_pos == bitcount(CwPat) * NbGroupsInBuf * dtu_size);
}


// send_data() sends next data packet.
// In tcl's result buffer, return
//    0, if current pass not yet finished
//   -1, if reached the end of the current pass 
int MFTPSndAgent::send_data()
{
    Packet* p = Agent::allocpkt();
    hdr_mftp* hdr = (hdr_mftp*) p->access(off_mftp_);
    CW_PATTERN_t mask;
    Tcl& tcl = Tcl::instance();

    assert(0 <= CurrentGroup && CurrentGroup < nb_groups);
    assert(NbGroupsInBuf >= 0);
    assert(0 <= MinGroupNbInBuf && MinGroupNbInBuf + NbGroupsInBuf <= nb_groups);

    // now comes NACK processing: loop until end of file or until
    // a nak bit is detected:
    while(CurrentGroup < nb_groups && IS_BIT_CLEARED(naks, CurrentGroup)) {
        CurrentGroup++;         // proceed to next bit of the nak bitmap
    }

    // do not transmit packet if
    // (1) CurrentGroup has reached the total number of groups ("end of pass") or
    // (2) CwPat has only bits set that refer to some packets that are cut off in
    //     the current group (for example, if CurrentGroup has only 5 packets
    //     with nb_groups=8 and if CwPat=64+32)
    if(CurrentGroup != nb_groups &&
       ((mask = (~(CW_PATTERN_t) 0) >> (8 * sizeof(CW_PATTERN_t) - get_dtus_per_group(CurrentGroup)))
        & CwPat) != 0) {
        assert(CurrentGroup < nb_groups);

        // see if the read-ahead-buffer is exhausted so that we must load new data
        // from file. Only groups with a corresponding NAK-bit are read, that is,
        // those that were requested for retransmission
        assert(MinGroupNbInBuf <= CurrentGroup);
        if(CurrentGroup >= MinGroupNbInBuf + NbGroupsInBuf) { // exhausted?
            fill_read_ahead_buf();               // load new data from file
        }
        assert(MinGroupNbInBuf <= CurrentGroup &&
               CurrentGroup < MinGroupNbInBuf + NbGroupsInBuf);

        // produce an encoded packet here (omitted)

        // generate the header
        hdr->type = hdr_mftp::PDU_DATA_TRANSFER;
        hdr->spec.data.pass_nb  = CurrentPass;
        hdr->spec.data.group_nb = CurrentGroup;
        hdr->spec.data.cw_pat   = CwPat & mask;

        char buf[8 * sizeof(CW_PATTERN_t) + 1];
        (CwPat & mask).print(buf);
        tcl.evalf("%s send notify %lu %lu %s",
                  name_,
                  (unsigned long) CurrentPass,
                  (unsigned long) CurrentGroup,
                  (char*) buf);

        hdr_cmn* ch = (hdr_cmn*) p->access(off_cmn_);
        ch->size() = sizeof(hdr_mftp) + dtu_size;

        // transmit packet
        target_->recv(p);

        RESET_BIT(naks, CurrentGroup); // reset the dtu status bit in the nak bitmap
        SET_BIT(retx, CurrentGroup);   // set the dtus status bit in the retransmission bitmap

        CurrentGroup++;
    } // if

    // if last group of the file:
    if(CurrentGroup == nb_groups || !(CwPat & mask)) { // end of pass?
        do {
            CwPat = iterator.getNextCwPat(); // get next codeword for new pass
        } while(!(CwPat & ((~(CW_PATTERN_t) 0) >> (8 * sizeof(CW_PATTERN_t) - get_dtus_per_group(0)))));
        //        } while(!(CwPat & ((((CW_PATTERN_t) 1) << get_dtus_per_group(0)) - 1)));

        // prepare a new pas:
        MinGroupNbInBuf = 0;
        NbGroupsInBuf = 0;
        CurrentGroup = 0;

        // reset retransmission bitmap for dealing with of latent naks
        RESET_ALL_BITS(retx, nb_groups);

        // the first dtus_per_group passes must be transmitted "in full"
        if(CurrentPass < dtus_per_group - 1) {
            SET_ALL_BITS(naks, nb_groups);
        }
        tcl.evalf("%s pass-finished %lu %lu", name_,
                  (unsigned long) CurrentPass,
                  (unsigned long) nb_blocks());
        CurrentPass++;
        tcl.result("-1");  // return end-of-pass to the caller

        return TCL_OK;
    }
    tcl.result("0");       // end-of-pass not yet reached
    return TCL_OK;
}


