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
 * File: codeword.h
 * Last change: Dec 14, 1998
 *
 * This software may freely be used only for non-commercial purposes
 */

#ifndef codeword_h
#define codeword_h

#ifndef WIN32
#include <unistd.h>   // due to definition of NULL
#endif /* !WIN32 */

class ExtraLongUInt {
public:
    ExtraLongUInt();
    ExtraLongUInt(const char* val);
    ExtraLongUInt(int val);
    ExtraLongUInt(unsigned int val);
    ExtraLongUInt(unsigned long val);
    bool          operator ==(const ExtraLongUInt& other) const;
    bool          operator !=(const ExtraLongUInt& other) const;
    bool          operator < (const ExtraLongUInt& other) const;
    bool          operator > (const ExtraLongUInt& other) const;
    bool          operator <=(const ExtraLongUInt& other) const;
    bool          operator >=(const ExtraLongUInt& other) const;
    ExtraLongUInt operator + (const ExtraLongUInt& other) const;
    ExtraLongUInt operator - (const ExtraLongUInt& other) const;
    ExtraLongUInt operator & (const ExtraLongUInt& other) const;
    ExtraLongUInt operator | (const ExtraLongUInt& other) const;
    ExtraLongUInt operator ^ (const ExtraLongUInt& other) const;
    ExtraLongUInt operator ~ () const;
    bool          operator !() const;
    ExtraLongUInt operator <<(unsigned int bits) const;
    ExtraLongUInt operator <<(const ExtraLongUInt& bits) const;
    ExtraLongUInt operator >>(unsigned int bits) const;
    ExtraLongUInt operator >>(const ExtraLongUInt& bits) const;
    const ExtraLongUInt& operator <<= (unsigned int bits);
    const ExtraLongUInt& operator >>= (unsigned int bits);
    const ExtraLongUInt& operator &=  (const ExtraLongUInt& other);
    const ExtraLongUInt& operator |=  (const ExtraLongUInt& other);
    const ExtraLongUInt& operator ^=  (const ExtraLongUInt& other);
    void print(char* buf) const;
    unsigned int bitcount() const;
    unsigned int minbit() const;
    enum { size = 2 };            // determines size of value (in units of unsigned long).
                                  // Adjust according to max. group size supported
protected:
    unsigned long value[size];
};


typedef ExtraLongUInt CW_PATTERN_t; // Codeword-typedef for erasure correction

typedef struct {                // Structure for the interna of erasure correction
  CW_PATTERN_t left;
  CW_PATTERN_t right;
} CW_MATRIXLINE_t;

unsigned int minbit(unsigned long val);
unsigned int bitcount(unsigned long val);
int initialize_codeword();

void init_bitcount_array(unsigned char* arr, unsigned int nb_bits);
void init_minbit_array(unsigned char* arr, unsigned int minbits);

class Codeword
{
public:
    Codeword();
    void setSourceWordLen(unsigned long k);
    CW_PATTERN_t getNextCwPat();
    CW_PATTERN_t getNextCwPatOLD(unsigned long dtus_per_group, CW_PATTERN_t cw_pat);

protected:
    enum { MAX_DEGREE = 63 };
    static CW_PATTERN_t primitive_polynomial[MAX_DEGREE+1];

    unsigned int k;
    CW_PATTERN_t n;
    CW_PATTERN_t cw_index;
    CW_PATTERN_t cw_pat;
    CW_PATTERN_t cw_saved;
};

unsigned int minbit(const ExtraLongUInt& val);
unsigned int bitcount(const ExtraLongUInt& val);

#endif
