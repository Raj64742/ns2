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
 * File: mftp_snd.h
 * Last change: Dec 14, 1998
 *
 * This software may freely be used only for non-commercial purposes
 */


// This file contains functionality specific to the MFTP sender.

#ifndef mftp_snd_h
#define mftp_snd_h

#include "codeword.h"
#include "mftp.h"              // due to class MFTPAgent



class MFTPSndAgent : public MFTPAgent {
public:
    MFTPSndAgent();
    virtual ~MFTPSndAgent();
    int command(int argc, const char*const* argv);
    void recv(Packet* p, Handler* h);

protected:
    void fill_read_ahead_buf(); // reads next couple of groups from disk into main memory
                                // note: the actual disk read is left out, since receiving the
                                // file is only simulated. We do count the number of disk seeks,
                                // though.

    void init_user_file(unsigned long readAheadBufsize);

    int send_data();            // sends next data packet
    void send_status_request(unsigned long pass_nb, unsigned long block_lo, unsigned long block_hi,
                             double txStatusDelay);
    void process_nak(hdr_mftp::Spec::Nak& nak, unsigned char* nak_bitmap,
                        unsigned long currentPass); // processes incoming NAK
    

    // note: the following variables are r/w from within a tcl-script:
    int readAheadBufsize_;      // this value is copied upon start of the sender protocol
    double txStatusDelay_;
    int nakCount_;

    // note: the following variables are not accessible within a tcl-script:
    unsigned char* naks;               // NAK bitmap, 0=>ACK, 1=>NAK
    unsigned char* retx;               // Retransmit bitmap, 0=>Not retx'd, 1=>retx'd
    unsigned long  fseek_offset;       // offset of next file read/write
    unsigned long  read_ahead_bufsize; // number of bytes read_ahead_buf consists of

    unsigned long  CurrentPass;        // current pass number
    unsigned long  CurrentGroup;       // current group number within pass
    CW_PATTERN_t   CwPat;              // used for erasure correction
    unsigned long  MinGroupNbInBuf;    // indicates lowest group number that is
                                       // currently cached in the read_ahead_buffer
    unsigned long  NbGroupsInBuf;      // number of groups currently stored in
                                       // the read_ahead_buffer
    Codeword iterator;                 // for iteration through sequence of codewords
};

#endif
