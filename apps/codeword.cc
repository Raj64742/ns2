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
 * File: codeword.cc
 * Last change: Nov. 14, 1997
 *
 * This software may freely be used only for non-commercial purposes
 */

#include "codeword.h"
#include "sb.h"

#include <assert.h>
#include <stdlib.h>  // due to definition of NULL

sb_uchar Codeword::minbit_array[256];
sb_uchar Codeword::bitcount_array[256];

// because of the lack of static initialization on class-basis in C++,
// this is a workaround.
static int dummy = Codeword::initialize_codeword();

CW_PATTERN_t Codeword::irreducible_polynom[Codeword::MAX_DEGREE+1] = {
    1,          // 1
    1+2,        // 1+x
    1+2+4,      // 1+x+x^2 
    1+2+8,           // 1+x+x^3
    1+2+16,          // 1+x+x^4
    1+4+32,          // 1+x^2+x^5
    1+2+4+32+64,     // 1+x+x^2+x^5+x^6
    1+4+32+64+128,   // 1+x^2+x^5+x^6+x^7
    1+8+32+64+256,          // 1+x^3+x^5+x^6+x^8
    1+2+16+256+512,         // 1+x^1+x^4+x^8+x^9
    1+4+8+64+256+512+1024,  // 1+x^2+x^3+x^6+x^8+x^9+x^10
    1+2+8+16+128+1024+2048, // 1+x+x^3+x^4+x^7+x^10+x^11
    1+8+16+32+128+512+4096, // 1+x^3+x^4+x^5+x^7+x^9+x^12
    1+8+16+32+64+4096+8192, // 1+x^3+x^4+x^5+x^6+x^12+x^13
    1+2+4+8+32+256+2048+8192+16384, // 1+x+x^2+x^3+x^5+x^8+x^11+x^13+x^14
    1+4+64+128+1024+16384+32768,    // 1+x^2+x^6+x^7+x^10+x^14+x^15
    1+4+16+64+1024+4096+16384+32768+65536 // 1+x^2+x^4+x^6+x^10+x^12+x^14+x^15+x^16
};

Codeword::Codeword() :
    k(1), n(1), cw_saved(0), cw_index(0), cw_pat(0)
{
}

sb_void Codeword::setSourceWordLen(sb_ulong k_)
{
    k = k_;
    n = ((CW_PATTERN_t) 1) << (k-1);
    cw_index = 0;
    cw_pat = 1;
    cw_saved = 0;
}

// systematic code, with optimization hack
CW_PATTERN_t Codeword::getNextCwPat()
{
    assert(cw_pat != 0); // or else prior call to setSourceWordLen(...) has not been made
    CW_PATTERN_t ret;
    CW_PATTERN_t cw_tmp;

    // step 1
    if(cw_index != 0) {
        cw_pat <<= 1;
        if(cw_pat >= n) {
            cw_pat ^= irreducible_polynom[k-1];
            assert(cw_pat < n);
        }
    }

    // step 2
    if(cw_index == k) { cw_saved = cw_pat; cw_tmp = n-1; }
    else if(cw_pat == n-1) cw_tmp = cw_saved;
    else cw_tmp = cw_pat;

    // step 3
    if(cw_index < k) ret = (CW_PATTERN_t) 1 << cw_index;
    else ret = (cw_tmp << 1) | 1;

    // step 4
    cw_index = (cw_index + 1) % n;

    assert(0 < ret && ret < ((CW_PATTERN_t) n << 1) | 1);
    return ret;
}

CW_PATTERN_t Codeword::getNextCwPatOLD(sb_ulong dtus_per_group, CW_PATTERN_t cw_pat)
{
    CW_PATTERN_t n_half = (CW_PATTERN_t) 1 << (dtus_per_group-1);

    assert(1 <= dtus_per_group && dtus_per_group <= MAX_DEGREE);
    assert(n_half != 0); /* if violated, data type CW_PATTERN_t is to small to incorporate */
    /* so many DTUs per block. Choose smaller group-size or make define CW_PATTERN_t to be */
    /* of a "larger" data type (i.e. unsigned long long) */

    if(cw_pat & n_half) {
        /* the polynom that is associated with cw_pat has "grown" to the same degree */
        /* as our irreducible polynom, i.e. degree "dtus_per_group" */

        /* do the shift left and lose possibly the MSB (if data type CW_PATTERN_t is */
        /* not large enough to still hold all bits */
        cw_pat <<= 1;

        /* perform reduction (polynomial division) over GF(2), i.e. XOR */
        /* (as divident and divident have the same degree) */
        /* In case the MSB got lost during the left-shift, the MSB of the */
        /* irreducible polynom also gets lost (as it is stored in the same data type), */
        /* thus either way, they sum to 0 (as must be the case) without further care. */
        cw_pat ^= irreducible_polynom[dtus_per_group];
    }
    else {
        cw_pat <<= 1;
    }
    return cw_pat;
}



bool Codeword::is_valid(CW_PATTERN_t cw, sb_ulong k)
{
    return true; // @@@@@
    /* note: if 1 gets shifted out of range, then we need the fact that */
    /* -1 == 0xFFFFFFF... Verfiy this behavious to avoid strange results: */
    //    assert(((CW_PATTERN_t) 1 << 8 * sizeof(CW_PATTERN_t)) - 1 == ~((CW_PATTERN_t) 0));

    assert(k <= 8 * sizeof(CW_PATTERN_t));
    return(cw <= ((CW_PATTERN_t) 1 << k) - 1); 
    // Ex: for k=8, highest
    // codeword is 255, although according to Reed-Muller order 1, only the odd
    // codewords are valid (but we don't check this for "manual" enhancements made
    // with the set of codewords.
}

sb_void Codeword::init_bitcount_array(sb_uchar* arr, sb_uint nb_bits)
{
    sb_uint32 bitcount_array_size = ((sb_uint32) 1) << nb_bits;
    sb_uint32 i;
    sb_uint bit;
    sb_int count;

    assert(0 < nb_bits && nb_bits <= 32); /* or else a revision is required */
    assert(arr != NULL);

    for(i = 0; i < bitcount_array_size; ++i) {
        count = 0;
        for(bit = 0;bit < nb_bits; bit++) {
            if(i & ((sb_uint32) 1 << bit)) {
                count++;
            } /* if */
        } /* for bit */
        arr[i] = count;
    } /* for i */
}

sb_void Codeword::init_minbit_array(sb_uchar* arr, sb_uint minbits)
{
    sb_uint minbit_array_size = (sb_uint) 1 << minbits;
    sb_uint i;
    sb_uint bit;

    assert(minbits == 8); /* or else minbit() needs a revision! */
    assert(arr != NULL);

    for(i = 0; i < minbit_array_size; ++i) {
        for(bit = 0;bit < minbits; bit++) {
            if(i & ((sb_uint) 1 << bit)) {
                arr[i] = (unsigned char) bit;
                break;
            } /* if */
        } /* for bit */
    } /* for i */
}

/* returns position of least significant bit in cw_pat */
sb_uint Codeword::minbit(CW_PATTERN_t cw_pat)
{
    sb_uint i;

    assert(cw_pat);
    assert(sizeof(sb_uchar) == 1);  /* or else this function needs a revision */
    for(i = 0; cw_pat; ++i) {
        assert(i < sizeof(CW_PATTERN_t));
        if(cw_pat & 255) {
            return minbit_array[(sb_uint) (cw_pat & 255)] + 8*i;
        }
        cw_pat >>= 8;
    }
    assert(0);
    return 0;
}

sb_uint Codeword::bit_count(CW_PATTERN_t cw_pat)
{
    sb_uint res = 0;

    while(cw_pat) {
        res += bitcount_array[cw_pat & 255];
        cw_pat >>= 8;
    }
    return res;
}

/* function to be called once at the beginning */
int Codeword::initialize_codeword()
{
    assert(sizeof(minbit_array) / sizeof(sb_uchar) == 256);
    init_minbit_array(minbit_array, 8);
    init_bitcount_array(bitcount_array, 8);
    return 0;
}


/* only needed for testing: */
#include <stdio.h>

int test()
{
    Codeword cw = Codeword();

    for(int k = 1; k < 9; ++k) {
        cw.setSourceWordLen(k);
        for(int i = 0; i <25; ++i) {
            printf("%lu ", (unsigned long) cw.getNextCwPat());
        }
        printf("\n");
    }
    exit(0);
}
