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
 * File: codeword.h
 * Last change: Nov. 14, 1997
 *
 * This software may freely be used only for non-commercial purposes
 */

#ifndef codeword_h
#define codeword_h

/*
 * The following sb_ definitions were pulled in from sb.h
 * to merge the mftp-specific .h's.
 * begin sb.h:
 */

typedef void            sb_void;
typedef unsigned        sb_unsigned;
typedef char            sb_char;
typedef char            sb_int8;
typedef unsigned char   sb_uchar;
typedef unsigned char   sb_uint8;
typedef int             sb_int;
typedef int             sb_long;
typedef unsigned int    sb_ulong;
typedef unsigned int    sb_uint;
typedef short           sb_short;
typedef short           sb_int16;
typedef unsigned short  sb_ushort;
typedef unsigned short  sb_uint16;
typedef long            sb_int32;
typedef unsigned long   sb_uint32;
typedef double          sb_double;

/* end sb.h: */


typedef sb_uint32 CW_PATTERN_t; /* Codeword-typedef for erasure correction  */

typedef struct {                /* Structure for the interna of erasure correction */
  CW_PATTERN_t left;
  CW_PATTERN_t right;
} CW_MATRIXLINE_t;


class Codeword
{
public:
    Codeword();
    sb_void setSourceWordLen(sb_ulong k);
    CW_PATTERN_t getNextCwPat();
    CW_PATTERN_t getNextCwPatOLD(sb_ulong dtus_per_group, CW_PATTERN_t cw_pat);
    static sb_uint minbit(CW_PATTERN_t cw_pat);
    static sb_uint bit_count(CW_PATTERN_t cw_pat);
    static int initialize_codeword();
    static bool is_valid(CW_PATTERN_t cw, sb_ulong k);

protected:
    enum { MAX_DEGREE = 16 };
    static sb_uchar minbit_array[256];
    static sb_uchar bitcount_array[256];
    static CW_PATTERN_t irreducible_polynom[MAX_DEGREE+1];
    static sb_void init_bitcount_array(sb_uchar* arr, sb_uint nb_bits);
    static sb_void init_minbit_array(sb_uchar* arr, sb_uint minbits);

    CW_PATTERN_t k;
    CW_PATTERN_t n;
    CW_PATTERN_t cw_saved;
    CW_PATTERN_t cw_index;
    //unsigned long long cw_index;
    CW_PATTERN_t cw_pat;
};

#endif
