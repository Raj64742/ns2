/* -*- c++ -*-
   marshall.h
   $Id: marshall.h,v 1.1 1998/12/08 19:09:50 haldar Exp $

   safely marshall and unmarshall 2 and 4 byte quantities into 
   possibly unaligned feilds

   NOTE: unlike the normal version of marshall, we marshall the bytes
   in *HOST*BYTE*ORDER* and *not* network byte order.  we do this since
   all the other quanities in the packets will be in host byte order, and
   we want consistency.

   */

#ifndef _cmu_marshall_h_
#define _cmu_marshall_h_

#ifdef sparc

#define GET4BYTE(x) (  (*(((unsigned char *)(x))  ) << 24) \
                    | (*(((unsigned char *)(x))+1) << 16) \
                    | (*(((unsigned char *)(x))+2) << 8) \
                    | (*(((unsigned char *)(x))+3)     ))

#define GET2BYTE(x) (  (*(((unsigned char *)(x))  ) << 8) \
                      | (*(((unsigned char *)(x))+1)     ))

/* let x,y be ptrs to ints, copy *x into *y */
#define STORE4BYTE(x,y) (((unsigned char *)y)[0] = ((unsigned char *)x)[0] ,\
		       ((unsigned char *)y)[1] = ((unsigned char *)x)[1] ,\
		       ((unsigned char *)y)[2] = ((unsigned char *)x)[2] ,\
		       ((unsigned char *)y)[3] = ((unsigned char *)x)[3] )

#define STORE2BYTE(x,y) (((unsigned char *)y)[0] = ((unsigned char *)x)[0] ,\
			 ((unsigned char *)y)[1] = ((unsigned char *)x)[1] )
		      

#else

#include <sys/types.h>

#define GET4BYTE(x) *((u_int32_t*) (x))

#define GET2BYTE(x) *((u_int16_t*) (x))

/* let x,y be ptrs to ints, copy *x into *y */
#define STORE4BYTE(x,y) (*((u_int32_t*) (x)) = *y)

#define STORE2BYTE(x,y) (*((u_int16_t*) (x)) = *y)

#endif


#endif /* _cmu_marshall_h_ */
