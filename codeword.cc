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
 * File: codeword.cc
 * Last change: Dec 07, 1998
 *
 * This software may freely be used only for non-commercial purposes
 */

#include "config.h" // for memcpy...
#include "codeword.h"
#include <assert.h>
#include <stdlib.h>  // due to definition of NULL
#include <stdio.h>   // due to printf

static unsigned char minbit_array[256];
static unsigned char bitcount_array[256];

// because of the lack of static initialization on class-basis in C++,
// this is a workaround.
static int dummy = initialize_codeword();

// list of primitive polynomials over GF(2).
CW_PATTERN_t Codeword::primitive_polynomial[Codeword::MAX_DEGREE+1] = {
    "1",         // 1 (group size 1)
    "11",        // 1+x
    "111",       // 1+x+x^2 
    "1011",      // 1+x+x^3  (group size 4)
    "10011",     // 1+x+x^4
    "100101",    // 1+x^2+x^5
    "1100111",   // 1+x+x^2+x^5+x^6
    "11100101",           // 1+x^2+x^5+x^6+x^7  (group size 8)
    "101101001",          // 1+x^3+x^5+x^6+x^8
    "1100010011",         // 1+x^1+x^4+x^8+x^9
    "11101001101",        // 1+x^2+x^3+x^6+x^8+x^9+x^10
    "110010011011",       // 1+x+x^3+x^4+x^7+x^10+x^11
    "1001010111001",      // 1+x^3+x^4+x^5+x^7+x^9+x^12
    "11000001111001",     // 1+x^3+x^4+x^5+x^6+x^12+x^13
    "110100100101111",    // 1+x+x^2+x^3+x^5+x^8+x^11+x^13+x^14
    "1100010011000101",   // 1+x^2+x^6+x^7+x^10+x^14+x^15 (group size 16)
    "11101010001010101",  // 1+x^2+x^4+x^6+x^10+x^12+x^14+x^15+x^16
    "101101011001011011", // 1+x+x^3+x^4+x^6+x^9+x^10+x^12+x^14+x^15+x^17
    "1101101001010011011",        // 1+x+x^3+x^4+x^7+x^9+x^12+x^14+x^15+x^17+x^18
    "11100101101010010011",
    "101010101101010010011",      // 1+x+x^4+x^7+x^9+x^11+x^12+x^14+x^16+x^18+x^20
    "1100010101110010011001",
    "10100100110001101011001",
    "101001011000010110110111",
    "1010101010100101110110001",
    "11001000110011010101011001",
    "100011000010001110100111011",      // group size: 27
    "1100110100111010101010100011",     // 1+x+x^5+x^7+x^9+x^11+x^13+x^15+x^16+x^17+x^20+x^22+x^23+x^26+x^27
    "10100110100111010101010100011",    // 1+x+x^5+x^7+x^9+x^11+x^13+x^15+x^16+x^17+x^20+x^22+x^23+x^26+x^28
    "100110011000111010101010100011",   // 1+x+x^5+x^7+x^9+x^11+x^13+x^15+x^16+x^17+x^21+x^22+x^25+x^26+x^29
    "1010100101000111010110100100011",  // 1+x+x^5+x^8+x^10+x^11+x^13+x^15+x^16+x^17+x^21+x^23+x^26+x^28+x^30
    "10110010100101100011010011011011", // 1+x+x^3+x^4+x^6+x^7+x^10+x^12+x^13+x^17+x^18+x^20+x^23+x^25+x^28+x^29+x^31 (group size 32)
    "0", // invalid, group size currently not supported
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "0",
    "1101100011100110100100111001001010110010100101010100100111011011" // 1+x+x^3+x^4+x^6+x^7+x^8+x^11+x^14+x^16+x^18+x^20+x^23+x^25+x^28+x^29+x^31+x^33+x^36+x^39+x^40+x^41+x^44+x^47+x^49+x^50+x^53+x^54+x^55+x^59+x^60+x^62+x^63 (group size 64)
};

// to generate primitive polynomials, see for example
//  http://www-kommsys.fernuni-hagen.de/~rieke/primitiv/test.phtml.en


Codeword::Codeword() :
    k(1),
    n(1),
    cw_index(0),
    cw_pat(0),
    cw_saved(0)
{
}

void Codeword::setSourceWordLen(unsigned long k_)
{
    k = k_;
    n = ((CW_PATTERN_t) 1) << (k-1);
    cw_index = (unsigned long) 0;
    cw_pat = 1;
    cw_saved = 0;
    assert(k <= 8 * sizeof(CW_PATTERN_t));
    if(k > MAX_DEGREE + 1 || primitive_polynomial[k-1] == 0) {
        fprintf(stderr, "codeword.cc: sorry, the group size %lu is not supported.\n",
                (unsigned long) k);
        exit(0);
    }
    if(k > 8 * sizeof(CW_PATTERN_t)) {
        fprintf(stderr, "codeword.cc: sorry, the group size %lu that you selected\n",
                (unsigned long) k);
        fprintf(stderr, "requires a datatype of at least %lu bits. To achieve this,\n",
                (unsigned long) k);
        fprintf(stderr, "adjust Codeword::size in file \"codeword.h\" accordingly.\n");
        exit(0);
    }
}

// systematic code, with optimization hack, originally derived from Reed-Muller codes of
// order 1.
CW_PATTERN_t Codeword::getNextCwPat()
{
    assert(cw_pat != 0); // or else prior call to setSourceWordLen(...) has not been made
    CW_PATTERN_t ret;
    CW_PATTERN_t cw_tmp;

    // step 1
    if(cw_index != 0) {
        cw_pat <<= 1;
        if(cw_pat >= n) {
            cw_pat ^= primitive_polynomial[k-1];
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
    cw_index = (cw_index + 1) & (n-1);    // "& (n-1)" is equivalent to "% n" here

    return ret;
}

void init_bitcount_array(unsigned char* arr, unsigned int nb_bits)
{
    unsigned long bitcount_array_size = ((unsigned long) 1) << nb_bits;
    unsigned long i;
    unsigned int bit;
    int count;

    assert(sizeof(unsigned long) == 4);   // we need this for the following
    assert(0 < nb_bits && nb_bits <= 32); // or else this function must be revised
    assert(arr != NULL);

    for(i = 0; i < bitcount_array_size; ++i) {
        count = 0;
	// to avoid warning: unsigned value >= 0 is always 1
	// for(bit = 0; 0 <= bit && bit < nb_bits; bit++) {
        for(bit = 0; bit < nb_bits; bit++) {
            if(i & ((unsigned long) 1 << bit)) {
                count++;
            }
        }
        arr[i] = count;
    }
}

void init_minbit_array(unsigned char* arr, unsigned int minbits)
{
    unsigned int minbit_array_size = (unsigned int) 1 << minbits;
    unsigned int i;
    unsigned int bit;

    assert(minbits == 8); // or else minbit(...) needs a revision!
    assert(arr != NULL);

    for(i = 0; i < minbit_array_size; ++i) {
      // to avoid warning: unsigned value >= 0 is always 1
      //for(bit = 0; 0 <= bit && bit < minbits; bit++) {
        for(bit = 0; bit < minbits; bit++) {
            if(i & ((unsigned int) 1 << bit)) {
                arr[i] = (unsigned char) bit;
                break;
            }
        }
    }
}

// returns bit position of least significant bit in cw_pat
unsigned int minbit(unsigned long val)
{
    unsigned int i;

    assert(val);
    for(i = 0; val; ++i) {
        assert(i < sizeof(CW_PATTERN_t));
        if(val & 255) {
            return minbit_array[(unsigned int) (val & 255)] + 8*i;
        }
        val >>= 8;
    }
    assert(false); // value is 0
    return 0;      // compiler, be quiet.
}

// counts number of bits in cw_pat
unsigned int bitcount(unsigned long val)
{
    unsigned int res = 0;

    while(val) {
        res += bitcount_array[(unsigned int) (val & 255)];
        val >>= 8;
    }
    return res;
}

/* function to be called once at the beginning */
int initialize_codeword()
{
    assert(sizeof(minbit_array) / sizeof(unsigned char) == 256);
    init_minbit_array(minbit_array, 8);
    init_bitcount_array(bitcount_array, 8);
    return 0;
}


// constructor:
ExtraLongUInt::ExtraLongUInt()
{
    memset(value, 0, sizeof(value));
}

// constructor: create from "binary-string" (string of 0's and 1's):
ExtraLongUInt::ExtraLongUInt(const char* val)
{
    const unsigned int len = strlen(val);
    char digit;

    assert(len <= 8 * sizeof(value));  // or else overflow
    memset(value, 0, sizeof(value));
    for(unsigned int i = 0; i < len; ++i) {
        digit = val[len - 1 - i];
        assert(digit == '0' || digit == '1'); // or else val is not a binary number
        if(digit == '1') {
            value[i / (8 * sizeof(unsigned long))] |=
                (unsigned long) 1 << (i % (8 * sizeof(unsigned long)));
        }
    }
}

// constructor:
ExtraLongUInt::ExtraLongUInt(int val)
{
    assert(val >= 0);
    memset(value, 0, sizeof(value));
    value[0] = val;
}

// constructor:
ExtraLongUInt::ExtraLongUInt(unsigned int val)
{
    memset(value, 0, sizeof(value));
    value[0] = val;
}

// constructor:
ExtraLongUInt::ExtraLongUInt(unsigned long val)
{
    memset(value, 0, sizeof(value));
    value[0] = val;
}

bool ExtraLongUInt::operator == (const ExtraLongUInt& other) const
{
    for(unsigned int i = 0; i < sizeof(value) / sizeof(unsigned long); ++i) {
        if(value[i] != other.value[i]) {
            return false;
        }
    }
    return true;
}

bool ExtraLongUInt::operator != (const ExtraLongUInt& other) const
{
    return (*this == other) ? false : true;
}

bool ExtraLongUInt::operator < (const ExtraLongUInt& other) const
{
    unsigned int i;

    for(i = sizeof(value) / sizeof(unsigned long) - 1;
        value[i] == other.value[i] && i > 0; --i) {
    }
    return value[i] < other.value[i] ? true : false;
}

bool ExtraLongUInt::operator > (const ExtraLongUInt& other) const
{
    unsigned int i;

    for(i = sizeof(value) / sizeof(unsigned long) - 1;
        value[i] == other.value[i] && i > 0; --i) {
    }
    return value[i] > other.value[i] ? true : false;
}

bool ExtraLongUInt::operator <= (const ExtraLongUInt& other) const
{
    return (*this > other) ? false : true;
}

bool ExtraLongUInt::operator >= (const ExtraLongUInt& other) const
{
    return (*this < other) ? false : true;
}

ExtraLongUInt ExtraLongUInt::operator + (const ExtraLongUInt& other) const
{
    ExtraLongUInt res = 0;
    unsigned long c = 0;
    const int shift = 8 * sizeof(unsigned long) - 1;
    const unsigned long msb_mask = (unsigned long) 1 << shift;
    const unsigned long lsbs_mask = ~msb_mask;
    unsigned long x, y;

    for(unsigned int i = 0; i < sizeof(value) / sizeof(unsigned long); ++i) {
        x = value[i];
        y = other.value[i];
        res.value[i] = (x & lsbs_mask) + (y & lsbs_mask) + c;
        c = ((x >> shift) + (y >> shift) + (res.value[i] >> shift)) >> 1;
        res.value[i] ^= (x & msb_mask) ^ (y & msb_mask);
    }
    return res;
}

ExtraLongUInt ExtraLongUInt::operator - (const ExtraLongUInt& other) const
{
    return *this + ~other + 1;
}

ExtraLongUInt ExtraLongUInt::operator & (const ExtraLongUInt& other) const
{
    ExtraLongUInt res;

    for(unsigned int i = 0; i < sizeof(value) / sizeof(unsigned long); ++i) {
        res.value[i] = value[i] & other.value[i];
    }
    return res;
}

ExtraLongUInt ExtraLongUInt::operator | (const ExtraLongUInt& other) const
{
    ExtraLongUInt res;

    for(unsigned int i = 0; i < sizeof(value) / sizeof(unsigned long); ++i) {
        res.value[i] = value[i] | other.value[i];
    }
    return res;
}

ExtraLongUInt ExtraLongUInt::operator ^ (const ExtraLongUInt& other) const
{
    ExtraLongUInt res;

    for(unsigned int i = 0; i < sizeof(value) / sizeof(unsigned long); ++i) {
        res.value[i] = value[i] ^ other.value[i];
    }
    return res;
}

ExtraLongUInt ExtraLongUInt::operator ~() const
{
    ExtraLongUInt res;

    for(unsigned int i = 0; i < sizeof(value) / sizeof(unsigned long); ++i) {
        res.value[i] = ~value[i];
    }
    return res;
}

bool ExtraLongUInt::operator ! () const
{
    for(unsigned int i = 0; i < sizeof(value) / sizeof(unsigned long); ++i) {
        if(value[i]) {
            return false;
        }
    }
    return true;
}

ExtraLongUInt ExtraLongUInt::operator << (unsigned int bits) const
{
    ExtraLongUInt res = 0;
    const int index =  bits / (8 * sizeof(unsigned long));
    const int shifts = bits % (8 * sizeof(unsigned long));
    unsigned int i;

    if(sizeof(value) > index * sizeof(unsigned long)) {
        if(shifts == 0) { // this is because (1 >> 32) is not 0 (gcc)
            memcpy(&res.value[index],
                   value,
                   sizeof(value) - index * sizeof(unsigned long));
        } else {
            const unsigned long mask = (~(unsigned long) 0) >> shifts;
            assert(sizeof(value) >= sizeof(unsigned long));
            res.value[index] = (value[0] & mask) << shifts;
            for(i = index + 1; i < sizeof(value) / sizeof(unsigned long); ++i) {
                res.value[i] = ((value[i - index  ] & mask) << shifts) |
                    (value[i - index-1]          >> ((8 * sizeof(unsigned long)) - shifts));
            }
        }
    }
    return res;
}

ExtraLongUInt ExtraLongUInt::operator >> (unsigned int bits) const
{
    ExtraLongUInt res = 0;
    const int index =  bits / (8 * sizeof(unsigned long));
    const int shifts = bits % (8 * sizeof(unsigned long));
    unsigned int i;

    if(sizeof(value) > index * sizeof(unsigned long)) {
        if(shifts == 0) { // this is because (1 >> 32) is not 0 (gcc)
            memcpy(res.value,
                   &value[index],
                   sizeof(value) - index * sizeof(unsigned long));
        } else {
            const unsigned long mask = (~(unsigned long) 0) >> (8 * sizeof(unsigned long) - shifts);
            assert(sizeof(value) >= sizeof(unsigned long));
            for(i = 0; i < sizeof(value) / sizeof(unsigned long) - index - 1; ++i) {
                res.value[i] = (value[i+index  ] >> shifts) |
                    ((value[i+index+1] & mask) << ((8 * sizeof(unsigned long)) - shifts));
            }
            res.value[i] = value[i+index] >> shifts;
        }
    }
    return res;
}

ExtraLongUInt ExtraLongUInt::operator << (const ExtraLongUInt& bits) const
{
    return *this << bits.value[0];
}

ExtraLongUInt ExtraLongUInt::operator >> (const ExtraLongUInt& bits) const
{
    return *this >> bits.value[0];
}

const ExtraLongUInt& ExtraLongUInt::operator <<= (unsigned int bits)
{
    *this = *this << bits;
    return *this;
}

const ExtraLongUInt& ExtraLongUInt::operator >>= (unsigned int bits)
{
    *this = *this >> bits;
    return *this;
}

const ExtraLongUInt& ExtraLongUInt::operator &= (const ExtraLongUInt& other)
{
    *this = *this & other;
    return *this;
}

const ExtraLongUInt& ExtraLongUInt::operator |= (const ExtraLongUInt& other)
{
    *this = *this | other;
    return *this;
}

const ExtraLongUInt& ExtraLongUInt::operator ^= (const ExtraLongUInt& other)
{
    *this = *this ^ other;
    return *this;
}

void ExtraLongUInt::print(char* buf) const
{
    int i, fin;

    for(i = 8 * sizeof(value) - 1; !(value[i / (8 * sizeof(unsigned long))] &
                          ((unsigned long) 1 << (i % (8 * sizeof(unsigned long))))) && i > 0; --i)
        ;
    for(fin = i; i >= 0; --i) {
        buf[fin-i] = (value[i / (8 * sizeof(unsigned long))] &
                     (unsigned long) 1 << (i % (8 * sizeof(unsigned long)))) ?
                     '1' : '0';
    }
    buf[fin+1] = '\0';
}

unsigned int ExtraLongUInt::bitcount() const
{
    unsigned int res = 0;
    for(unsigned int i = 0; i < sizeof(value) / sizeof(unsigned long); ++i) {
        res += ::bitcount(value[i]);
    }
    return res;
}

unsigned int ExtraLongUInt::minbit() const
{
    for(unsigned int i = 0; i < sizeof(value) / sizeof(unsigned long); ++i) {
        if(value[i]) {
            return i * 8 * sizeof(unsigned long) + ::minbit(value[i]);
        }
    }
    assert(false); // value is 0
    return 0;      // compiler, be quiet.
}



// returns bit position of least significant bit in cw_pat
unsigned int minbit(const ExtraLongUInt& val)
{
    return val.minbit();
}

// returns bit position of least significant bit in cw_pat
unsigned int bitcount(const ExtraLongUInt& val)
{
    return val.bitcount();
}
