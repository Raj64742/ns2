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
 * File: mftp_rcv.h
 * Last change: Dec 07, 1998
 *
 * This software may freely be used only for non-commercial purposes
 */


// This file contains functionality specific to an MFTP receiver.

#ifndef mftp_rcv_h
#define mftp_rcv_h

#include "codeword.h"
#include "mftp.h"                        // due to class MFTPAgent


class MFTPRcvAgent : public MFTPAgent {
public:
    MFTPRcvAgent();
    ~MFTPRcvAgent();
    int command(int argc, const char*const* argv);
    void recv(Packet* p, Handler* h);
    
protected:
    typedef struct {
        CW_PATTERN_t left;
        CW_PATTERN_t right;
    } CW_MATRIXLINE_t;

    void init();
    void addLine(unsigned long dtu_nb_from, unsigned long dtu_nb_to);
    int  process_packet(CW_PATTERN_t cw_pat,
                        unsigned long group_nb, unsigned long dtu_nb);
    int  findStoreLocation(unsigned long group_nb, unsigned long seek_offset, unsigned long* dtu_nb);
    void cw_matrixlines_reset();
    bool is_group_full(unsigned long group_nb);
    int  recv_data(hdr_mftp::Spec::Data& data);
    void recv_status_req(hdr_mftp::Spec::StatReq& statreq);
    void send_nak(unsigned long pass_nb, unsigned long block_nb);

    // The following variables are accessible from tcl-scripts:

    // note: ns uses dst_ as the multicast address from which packets are received,
    // thus we need the (unicast) reply address as well.
    ns_addr_t reply_;		        // unicast reply-address for status response messages

    // The following variables are not accessible from tcl-scripts:
    unsigned long CurrentPass;           // current pass number
    unsigned long CurrentGroup;          // current group number within pass
    CW_PATTERN_t  CwPat;                 // current codeword pattern within pass
    unsigned long FileDGramsReceived;    // number of datagrams fully received so far
    unsigned long FseekOffset;           // current fseek pointer
    CW_MATRIXLINE_t* cw_matrixline_buf; // Enables receiver to keep track which coded packets
                                        // were received and how to interpret the coding.
                                        // Buffer has as many entries as there are packets
                                        // in the file (i.e. FileDGrams)
};

#endif
