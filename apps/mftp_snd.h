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
 * File: mftp_snd.h
 * Last change: Jan. 13, 1998
 *
 * This software may freely be used only for non-commercial purposes
 */


#ifndef mftp_snd_h
#define mftp_snd_h

#include "sb.h"
#include "codeword.h"
#include "mftp.h"                    // due to class MFTPAgent

typedef struct NM_STATS_SND_t
{
    sb_ulong CurrentPass;            // current pass number
    sb_ulong CurrentGroup;           // current group number within pass
    CW_PATTERN_t CwPat;              // used for erasure correction
    sb_ulong MinGroupNbInBuf;        // indicates lowest group number that is
                                     // currently cached in the read_ahead_buffer
    sb_ulong NbGroupsInBuf;          // number of groups currently stored in
                                     // the read_ahead_buffer
    Codeword iterator;               // for iteration through sequence of codewords
    NM_STATS_SND_t() : CurrentGroup(0), MinGroupNbInBuf(0), NbGroupsInBuf(0),
        CwPat(0), CurrentPass(0) { };
} NM_STATS_SND_t;


class MFTPSndAgent : public MFTPAgent {
public:
    MFTPSndAgent();
    int command(int argc, const char*const* argv);
    void recv(Packet* p, Handler* h);

protected:
    sb_int fill_read_ahead_buf(); // reads next couple of groups from into main memory
                                  // note: the actual disk read is left out in this simulation

    sb_int user_file_init(sb_ulong filesize, sb_ulong dtusize,
                          sb_ulong dtusPerBlock, sb_ulong dtusPerGroup,
                          sb_ulong readAheadBufsize);

    sb_int send_data();           // sends next data packet
    sb_void send_status_request(sb_ulong pass_nb, sb_ulong block_lo, sb_ulong block_hi,
                                double txStatusDelay);
    sb_void process_nak(hdr_mftp::Spec::Nak& nak, sb_uint8* nak_bitmap,
                        sb_ulong currentPass); // processes incoming NAK
    

    // note: the following variables are r/w from within a tcl-script:

    int readAheadBufsize_; // may NOT be changed during file-transfer
    double txStatusDelay_; // time to wait for NAKs at end of round (may be changed during file-transfer)
    int nakCount_;         // counts number of NAKs recvd at end of round (may be changed during file-transfer)

    // note: the following variables are not accessible within a tcl-script:
    sb_ulong  nak_bytes;              // number of significant bytes in "naks" below
    sb_ulong  last_nak;               // offset of last nak byte
    sb_char   last_nak_mask;          // mask image for last nak byte
    sb_uint8* naks;                   // NAK bitmap, 0=>ACK, 1=>NAK
    sb_uint8* retx;                   // Retransmit bitmap, 0=>Not retx'd, 1=>retx'd
    sb_ulong  fseek_offset;           // offset of next file read/write
    sb_ulong  read_ahead_bufsize;     // number of bytes read_ahead_buf consists of
    NM_STATS_SND_t nmstats;
};

#endif
