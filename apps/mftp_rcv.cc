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
 * File: mftp_rcv.cc
 * Last change: Dec 14, 1998
 *
 * This software may freely be used only for non-commercial purposes
 */

// This file contains functionality specific to an MFTP receiver.


#include <assert.h>

#include "Tcl.h"
#include "mftp_rcv.h"

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
      CurrentPass(0),
      CurrentGroup(0),
      CwPat(0),
      FileDGramsReceived(0),
      FseekOffset(0),
      cw_matrixline_buf(NULL)
{
    bind("reply_", (int*)&reply_);
}

// inspect a Tcl command (overloaded function):
int MFTPRcvAgent::command(int argc, const char*const* argv)
{
    Tcl& tcl = Tcl::instance();
    if(strcmp(argv[1], "send") == 0) {
        if(strcmp(argv[2], "nak") == 0) {
            unsigned long pass_nb, block_nb;
            int nb_scanned = 0;

            nb_scanned += sscanf(argv[3], "%lu", &pass_nb);
            nb_scanned += sscanf(argv[4], "%lu", &block_nb);
            assert(nb_scanned == 2);
            send_nak(pass_nb, block_nb); 
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
        // packet from local agent
        fprintf(stderr, "%s: send not allowed with Agent/MFTP/Rcv\n", name_);
        assert(false);
    } else {
        switch(mh->type) {
        case hdr_mftp::PDU_DATA_TRANSFER:
            recv_data(mh->spec.data);
            break;
        case hdr_mftp::PDU_STATUS_REQUEST:
            recv_status_req(mh->spec.statReq);
            break;
        case hdr_mftp::PDU_NAK:
            // as we are a member of the group as well, we receive all data we have sent.
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
}


void MFTPRcvAgent::init()
{
    MFTPAgent::init();

    // allocate cw_matrix_line_buf
    assert(cw_matrixline_buf == NULL);
    cw_matrixline_buf = new CW_MATRIXLINE_t[FileDGrams];
    assert(cw_matrixline_buf != NULL);  // or else no memory is left!
    // should return an error instead of terminating the program

    // reset array:
    cw_matrixlines_reset();
}


/* process a received status request packet */
void MFTPRcvAgent::recv_status_req(hdr_mftp::Spec::StatReq& statreq)
{
    Tcl& tcl = Tcl::instance();

    // read the PDU_STATUS_REQUEST-specific fields:
    tcl.evalf("%s recv status-req %lu %lu %lu %lf", name_,
              (unsigned long) statreq.pass_nb,
              (unsigned long) statreq.block_lo,
              (unsigned long) statreq.block_hi,
              (double) statreq.RspBackoffWindow);
}

// send a nak packet:
void MFTPRcvAgent::send_nak(unsigned long pass_nb, unsigned long block_nb)
{
    assert(FileDGrams > 0);
    assert(0 <= block_nb && block_nb < nb_blocks());

    Tcl& tcl = Tcl::instance();

    // start_group_nb corresponds to first bit of NAK-bitmap:
    unsigned long start_group_nb = dtus_per_block * block_nb;

    // end_group_nb corresponds to last group number of NAK-bitmap plus one
    unsigned long end_group_nb = min(nb_groups, dtus_per_block * (block_nb + 1));

    // number of valid bits in the outgoing nak-bitmap
    unsigned long n = end_group_nb - start_group_nb;

    // number of status bytes in pdu
    const unsigned long nak_bytes = (n+7) / 8;

    unsigned long bit_count = 0;

    // allocate (get) new packet and dynamically allocate extra space for nak-bitmap:
    Packet* p = Agent::allocpkt((n+7) / 8);

    unsigned char* nak_bitmap = (unsigned char*) p->accessdata();

    // clear NAK-bitmap first:
    memset(nak_bitmap, 0, nak_bytes);
    
    // loop over all groups in rangs of nak and set nak-bit for those that are not still full
    for(unsigned long group_nb = start_group_nb, bit = 1 << (start_group_nb % 8);
        group_nb < end_group_nb; ++group_nb) {
        if(is_group_full(group_nb) == false) {
            *nak_bitmap |= bit;
            bit_count++;
        }
        if(bit == 128) {
            bit = 1;
            nak_bitmap++;
        } else {
            bit <<= 1;
        }
    }

    if(bit_count > 0) {
        hdr_ip* iph = (hdr_ip*)p->access(off_ip_);
        hdr_mftp* hdr = (hdr_mftp*) p->access(off_mftp_);
        hdr_cmn* ch = (hdr_cmn*) p->access(off_cmn_);

        // now generate the header
	iph->dst() = reply_;    // overwrite settings from Agent::allocpkt()
        ch->size() = sizeof(hdr_mftp);

        hdr->type = hdr_mftp::PDU_NAK;
        hdr->spec.nak.pass_nb   = pass_nb;
        hdr->spec.nak.block_nb  = block_nb;
        hdr->spec.nak.nak_count = bit_count;

        // transmit packet
        target_->recv(p);
    }
    else {
        Packet::free(p);  // do not transmit NAK-packet if it consists of 0 NAK-bits !!
                          // HACK: @ requires optimation still!
    }
    tcl.resultf("%lu", bit_count);
}

// process_packet: decides if a received packet is "useful" and
// stores meta-information for proper decoding
int MFTPRcvAgent::process_packet(CW_PATTERN_t cw_pat,
                                 unsigned long group_nb, unsigned long dtu_nb)
{
    CW_PATTERN_t bit;
    CW_MATRIXLINE_t new_row;

    unsigned long j;    // j iterates over the dtus of group "group_nb"
    unsigned long finish = get_dtus_per_group(group_nb);
                        // finish counts the number of dtus in group "group_nb"

    new_row.left = cw_pat;

    for(j = 0; j < finish; j++) {
        CW_PATTERN_t line_pat = cw_matrixline_buf[j * nb_groups + group_nb].left;
        if(line_pat != 0) {
            bit = new_row.left & ((CW_PATTERN_t) 1 << minbit(line_pat));
            if(bit != 0) {
                new_row.left ^= line_pat;
            }
        }
    }
    if(new_row.left != 0) { // linear independent?
        bit = (CW_PATTERN_t) 1 << minbit(new_row.left);
        for(j = 0; j < finish; j++) {
            if((bit & cw_matrixline_buf[j * nb_groups + group_nb].left) != 0) {
                cw_matrixline_buf[j * nb_groups + group_nb].left ^= new_row.left;
            }
        }
        // register pattern of codeword the received packet is composed of (possibly altered).
        // must be done at last for that this line gets not erased by XORing with itself
        // in the previous loop.
        cw_matrixline_buf[dtu_nb * nb_groups + group_nb].left = new_row.left;
        return 1; // packet was a "useful" packet (i.e. is linear independent
                  // from the other ones received so far)
    }
    else {
        return 0; //linear dependent codeword-pattern received, i.e. useless
    }
}


int MFTPRcvAgent::findStoreLocation(unsigned long group_nb, unsigned long seek_offset, unsigned long* dtu_nb)
{
    unsigned long start_dtu_nb;

    assert(0 <= group_nb && group_nb < nb_groups);
    assert(seek_offset % dtu_size == 0 ||
           seek_offset == FileSize);

    if(seek_offset == FileSize) {
        *dtu_nb = group_nb;    // start over from the beginning
    } else {
        unsigned long curr_dtu_nb = FseekOffset / dtu_size;

        // pay attention to "unsigned" when substracting
        *dtu_nb = curr_dtu_nb - curr_dtu_nb % nb_groups;
        *dtu_nb += group_nb;

        // check if seeking backwards. If yes, increment dtu_nb by nb_groups to
        // always seeks forwards (unless end of file is reached):
        if(*dtu_nb < curr_dtu_nb) {
            *dtu_nb += nb_groups;
        }
    }
    if(*dtu_nb >= FileDGrams) {
        // this might happen if some groups have less packets than
        // dtus_per_group:
        *dtu_nb = group_nb;    // start over from the beginning
    }
    start_dtu_nb = *dtu_nb;
    assert(start_dtu_nb < FileDGrams);

    do {
        if(! cw_matrixline_buf[*dtu_nb].left) {
            return 1;
        }
        *dtu_nb += nb_groups;
        if(*dtu_nb >= FileDGrams) {
            *dtu_nb = group_nb;    // start over from the beginning
        }
    } while(*dtu_nb != start_dtu_nb);
    return 0;    // group "group_nb" is already full
}


// initializes all matrix-lines to zero, i.e. no (encoded) packets
// at all are received so far:
void MFTPRcvAgent::cw_matrixlines_reset()
{
    assert(0 <= FileDGrams);
    memset(cw_matrixline_buf, 0, sizeof(CW_MATRIXLINE_t) * FileDGrams);
}


// returns true if group "group_nb" is full, false if there is at least one packet missing
bool MFTPRcvAgent::is_group_full(unsigned long group_nb)
{
    unsigned long nb_dtus = get_dtus_per_group(group_nb);
    unsigned long i;

    assert(0 <= group_nb && group_nb < nb_groups);

    for(i = 0; i < nb_dtus &&
            cw_matrixline_buf[i * nb_groups + group_nb].left != 0; ++i)
        ;
    return (i == nb_dtus) ? true : false; // if loop was left before nb_dtus was reached,
                                          // then there is some line in the matrix that is
                                          // all "0", i.e. a packet is still missing.
}

// recv_data: process received data packet;
// takes (received) coded packets and processes them.
int MFTPRcvAgent::recv_data(hdr_mftp::Spec::Data& data)
{
    Tcl& tcl = Tcl::instance();
    unsigned long seek_offset;
    unsigned long dtu_nb; // position (in terms of datagram number) where incoming
                          // packet is stored in file
    
    // read the PDU_DATA_TRANSFER-specific fields:
    CurrentPass  = data.pass_nb;
    CurrentGroup = data.group_nb;
    CwPat = data.cw_pat;

    // validate fields:
    // (actually, assert should be replaced by just ignoring the packet in case
    // the parameters are invalid, as some corrupt packet might reach the receiver
    // in real world, i.e. this would not be a bug in the software!)
    assert(0 <= CurrentPass);
    assert(0 <= CurrentGroup && CurrentGroup < nb_groups);


    if(findStoreLocation(CurrentGroup, FseekOffset, &dtu_nb)) {
        // arriving packet belongs to a not already full group:
        assert(dtu_nb % nb_groups == CurrentGroup);
        assert(0 <= dtu_nb && dtu_nb < FileDGrams);

        if(process_packet(CwPat,
                          CurrentGroup,
                          dtu_nb / nb_groups)) {
            cw_matrixline_buf[dtu_nb].right = CwPat;
            // arriving packet is useful (i.e. linearly independent from the others
            // of the group, thus store packet on disk:

            char buf[8 * sizeof(CW_PATTERN_t) + 1];
            CwPat.print(buf);
            tcl.evalf("%s recv useful %lu %lu %s",
                      name_,
                      (unsigned long) CurrentPass,
                      (unsigned long) CurrentGroup,
                      (char*) buf);

                
            seek_offset = dtu_nb * dtu_size;
            if(dtu_nb == FileDGrams - 1) {
                // the last dtu of the file might not fit into the file, as with
                // erasure correction, the dtu size must always be the same,
                // i.e. dtu_size. So we don't write the packet on disk.
                // Rather, we store it in a special place in main memory.
                // (ommitted)
            } else {
                // prepare to write the new packet to the file system:
                if(FseekOffset != seek_offset) {
                    // seek to file-position seek_offset (omitted)
                    FseekOffset = seek_offset;
                    seekCount_++;
                }
                // write data to file here (omitted)
                FseekOffset += dtu_size;
            } // else
            // increment number of good dtus received
            FileDGramsReceived++;
            
            // if all packets have been received, decode the file and send a done-message
            if (FileDGramsReceived == FileDGrams) {
                // decode file here. Involves the file, the cw_matrixline_buf-array and
                // the last packet (cached in memory). Additional disk activity for the
                // receivers will be required.
                // (omitted)
                char buf[8 * sizeof(CW_PATTERN_t) + 1];
                CwPat.print(buf);
                tcl.evalf("%s done-notify %lu %lu %s",
                          name_,
                          (unsigned long) CurrentPass,
                          (unsigned long) CurrentGroup,
                          (char*) buf);
                return(0);   // we are ready!
            }
        } // if(process_packet...)
        else {
            char buf[8 * sizeof(CW_PATTERN_t) + 1];
            CwPat.print(buf);
            tcl.evalf("%s recv dependent %lu %lu %s",
                      name_,
                      (unsigned long) CurrentPass,
                      (unsigned long) CurrentGroup,
                      (char*) buf);
            return(0);   // we are ready!
        }                
    } // if(findStoreLocation...)
    else {
        // we received a packet that belongs to an already full group
        char buf[8 * sizeof(CW_PATTERN_t) + 1];
        CwPat.print(buf);
        tcl.evalf("%s recv group-full %lu %lu %s",
                  name_,
                  (unsigned long) CurrentPass,
                  (unsigned long) CurrentGroup,
                  (char*) buf);
    }
    return(0);
}
