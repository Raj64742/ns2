
/*
 * bitops.h
 *
 * These functions were originally from Christoph Haenle, chris@cs.vu.nl
 * but the were generalized to not require sb_* definitions.
 *
 */

#ifndef BITOPS_H
#define BITOPS_H

#include "string.h"      /* due to memset */

/* we'll undef these before leaving this .h */
#define sb_uint8 unsigned char
#define sb_ulong unsigned long

/* determines if bit number "bit_nb" is set in array "arr" */
#define IS_BIT_SET(arr, bit_nb) (((sb_uint8*) arr)[(bit_nb) >> 3] & (((sb_uint8) 1) << ((bit_nb) & 7)))

/* determines if bit number "bit_nb" is set in array "arr" */
#define IS_BIT_CLEARED(arr, bit_nb) (! IS_BIT_SET(arr, bit_nb))

/* resets bit "bit_nb" in array "arr" */
#define RESET_BIT(arr, bit_nb) (((sb_uint8*) arr)[(bit_nb) >> 3] &= ~(((sb_uint8) 1) << ((bit_nb) & 7)))

/* sets bit "bit_nb" in array "arr" */
#define SET_BIT(arr, bit_nb)   (((sb_uint8*) arr)[(bit_nb) >> 3] |=   ((sb_uint8) 1) << ((bit_nb) & 7))



/* set the first nb_bits in array arr */
inline void SET_ALL_BITS(sb_uint8* arr, sb_ulong nb_bits)
{
    memset(arr, 255, nb_bits >> 3);
    if(nb_bits & 7) {
        arr[nb_bits >> 3] |= ((sb_uint8) 255) >> (8 - (nb_bits & 7));
    }
}

/* reset the first nb_bits in array arr */
inline void RESET_ALL_BITS(sb_uint8* arr, sb_ulong nb_bits)
{
    memset(arr, 0, nb_bits >> 3);
    if(nb_bits & 7) {
        arr[nb_bits >> 3] &= ((sb_uint8) 255) << (nb_bits & 7);
    }
}

#undef sb_uint8
#undef sb_ulong

#endif
