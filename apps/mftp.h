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
 * File: mftp.h
 * Last change: Dec 07, 1998
 *
 * This software may freely be used only for non-commercial purposes
 */

// This file contains functionality common to both MFTP sender and receivers.

#ifndef mftp_h
#define mftp_h

#include "codeword.h"  // due to definition of CW_PATTERN_t
#include "agent.h"     // due to class Agent
#include "assert.h"    // due to assert()


class hdr_mftp {
public:
    enum { PDU_DATA_TRANSFER, PDU_STATUS_REQUEST, PDU_NAK }; // allowed types of PDU's

    int type;          // field for PDU-type
    // instead of "class Spec", should use "union Spec", but doesn't work
    // since CW_PATTERN_t is a class.
    class Spec {       // specific part of the PDU (different for different types)
    public:
        class Data {            // PDU_DATA_TRANSFER:
        public:
            unsigned long pass_nb;
            unsigned long group_nb;
            CW_PATTERN_t cw_pat;
        } data;
        class StatReq {         // PDU_STATUS_REQUEST:
        public:
            unsigned long pass_nb;     // pass number the status request belongs to
            unsigned long block_lo;    // lowest block number requested for NAK-feedback
            unsigned long block_hi;    // highest block number requested
            double   RspBackoffWindow; // length of backoff-interval that receivers
            // are supposed to use when generating a random time (from a uniform
            // distribution) before (potentially) sending NACKs.
        } statReq;
        class Nak {             // PDU_NAK:
        public:
            unsigned long pass_nb;
            unsigned long block_nb;
            unsigned long nak_count;
            // the actual nak-bitmap will be found in a variable length field that
            // is dynamically allocated.
        } nak;
    } spec;
};

class MFTPAgent : public Agent {
protected:
    MFTPAgent();
    int init();
    
    // the following variables are read/write from within a tcl-script:
    int dtuSize_;
    int fileSize_;
    int dtusPerBlock_;
    int dtusPerGroup_;
    int seekCount_;
    
    int off_mftp_;
    int off_cmn_;

    // the following variables are not accessible from tcl-scripts:
    unsigned long FileSize;           // size of file of this transfer
    unsigned long FileDGrams;         // number of datagrams in this transfer
    unsigned long dtu_size;           // number of data bytes in a DTU
    unsigned long dtus_per_block;     // number of DTUs per block
    unsigned long dtus_per_group;     // number of DTUs per group
    unsigned long nb_groups;          // number of groups the file consists of

    unsigned long nb_blocks() const;
    unsigned long get_dtus_per_group(unsigned long group_nb) const;
};

inline unsigned long MFTPAgent::nb_blocks() const
{
    return (nb_groups+dtus_per_block-1)/dtus_per_block;
}

inline unsigned long MFTPAgent::get_dtus_per_group(unsigned long group_nb) const
{
    assert(0 <= group_nb && group_nb < nb_groups);
    assert(nb_groups > 0);

    unsigned long res = FileDGrams / nb_groups;

    if(group_nb < FileDGrams % nb_groups) {
        res++;
    }
    assert(0 <= res && res <= dtus_per_group);
    assert(res == 0 || FileDGrams > 0);
    return res;
}

#endif
