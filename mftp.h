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
 * File: mftp.h
 * Last change: Dec. 15, 1997
 *
 * This software may freely be used only for non-commercial purposes
 */


#ifndef mftp_h
#define mftp_h

#include "sb.h"        // due to definition of sb_-data types
#include "codeword.h"  // due to definition of CW_PATTERN_t
#include "agent.h"     // due to class Agent
#include "assert.h"    // due to assert()


struct hdr_mftp {
    enum { PDU_DATA_TRANSFER = 5, PDU_STATUS_REQUEST = 6, PDU_NAK = 7 }; // allowed types of PDU's
    // (we start numbering with 5 to be conform to the MFTP-spec)

    sb_int16 type;     // field for PDU-type
    union Spec {       // specific part of the PDU (different for different types)
        struct Data {            // PDU_DATA_TRANSFER:
            sb_ulong pass_nb;
            sb_ulong group_nb;
            CW_PATTERN_t cw_pat;
        } data;
        struct StatReq {         // PDU_STATUS_REQUEST:
            sb_ulong pass_nb;    // pass number he status request belongs to
            sb_ulong block_lo;   // lowest block number requested for NAK-feedback
            sb_ulong block_hi;   // highest block number requested
            sb_ulong group_lo;   // lowest group number requested
            sb_ulong group_hi;   // highest group number requested
            sb_double RspBackoffWindow; // length of backoff-interval that clients
            // are supposed to use when generating a random time (from a uniform
            // distribution) before (potentially) sending NACKs.
        } statReq;
        struct Nak {             // PDU_NAK:
            sb_ulong pass_nb;
            sb_ulong block_nb;
            sb_ulong nak_count;
            // nak-bitmap can be found in a variable len field that gets dynamically
            // allocated.
        } nak;
    } spec;
};

class MFTPAgent : public Agent {
protected:
    MFTPAgent();
    int init();
    
    // Note: the following variables are r/w from within a tcl-script:
    int dtuSize_;
    int fileSize_;
    int dtusPerBlock_;
    int dtusPerGroup_;
    int seekCount_;
    
    int off_mftp_;
    int off_cmn_;

    // The following variables are not accessible from tcl-scripts:
    sb_ulong FileSize;           // size of this file
    sb_ulong FileDGrams;         // number of datagrams in this transfer
    sb_ulong dtu_size;           // number of data bytes in a DTU
    sb_ulong dtus_per_block;     // number of DTUs per block
    sb_ulong dtus_per_group;     // number of DTUs per group
    sb_ulong end_dtu_size;	 // size of last dtu in transfer
    sb_ulong nb_groups;          // number of groups the file consists of

    sb_ulong nb_blocks() const;
    sb_ulong get_dtus_per_group(sb_ulong group_nb) const;

};
    
inline sb_ulong MFTPAgent::nb_blocks() const
{
    return (nb_groups+dtus_per_block-1)/dtus_per_block;
}

inline sb_ulong MFTPAgent::get_dtus_per_group(sb_ulong group_nb) const
{
    assert(0 <= group_nb && group_nb < nb_groups);
    assert(nb_groups > 0);

    sb_ulong res = FileDGrams / nb_groups;

    if(group_nb < FileDGrams % nb_groups) {
        res++;
    }
    assert(0 <= res && res <= dtus_per_group);
    assert(res == 0 || FileDGrams > 0);
    return res;
}

#endif
