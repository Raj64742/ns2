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
 * File: mftp_rcv.h
 * Last change: Jan. 13, 1998
 *
 * This software may freely be used only for non-commercial purposes
 */


#ifndef mftp_rcv_h
#define mftp_rcv_h

#include "codeword.h"
#include "mftp.h"                       // due to MFTPAgent

typedef struct NM_STATS_RCV_t
{
    sb_ulong     CurrentGroup;          // current group number within pass
    CW_PATTERN_t CwPat;                 // used for erasure correction
    sb_ulong     CurrentPass;           // current pass number
    sb_ulong     FileDGramsReceived;    // number of datagrams fully received
    sb_ulong     FseekOffset;           // offset of next file read/write
    NM_STATS_RCV_t() : CurrentGroup(0), CwPat(0),
        CurrentPass(0), FileDGramsReceived(0), FseekOffset(0) { };
} NM_STATS_RCV_t;


class MFTPRcvAgent : public MFTPAgent {
public:
    MFTPRcvAgent();
    ~MFTPRcvAgent();
    int command(int argc, const char*const* argv);
    void recv(Packet* p, Handler* h);

protected:
    typedef sb_uint32 CW_PATTERN_t;

    typedef struct {
        CW_PATTERN_t left;
        CW_PATTERN_t right;
    } CW_MATRIXLINE_t;

    sb_void init();
    sb_void addLine(sb_ulong dtu_nb_from, sb_ulong dtu_nb_to);
    sb_int  process_frame_no(CW_PATTERN_t cw_pat,
                             sb_ulong group_nb, sb_ulong dtu_nb);
    sb_int  findStoreLocation(sb_ulong group_nb, sb_ulong seek_offset, sb_ulong* dtu_nb);
    sb_void cw_matrixlines_reset();
    sb_int  group_full(sb_ulong group_nb);
    sb_int  recv_data(hdr_mftp::Spec::Data& data);
    sb_void recv_status_req(hdr_mftp::Spec::StatReq& statreq);
    sb_void send_nak(sb_ulong pass_nb, sb_ulong block_nb);

    // The following variables are accessible from tcl-scripts:

    // note: ns uses dst_ as the multicast address from which packets are received,
    // thus we need the (unicast) reply address as well.
    nsaddr_t reply_;		        // unicast reply-address for status response messages

    // The following variables are not accessible from tcl-scripts:
    NM_STATS_RCV_t nmstats;
    CW_MATRIXLINE_t* cw_matrixline_buf; // Enables receiver to keep track which coded frames
                                        // were received and how to interpret the coding
                                        // Buffer has so many entries as there are frames
                                        // in the file (i.e. FileDGrams)
    sb_uchar*   last_dtu_buf;           // stores last dtu
};

#endif
