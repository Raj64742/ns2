//
// Copyright (c) 1997 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//

#ifndef ns_srm_state_h
#define	ns_srm_state_h

#include <assert.h>

class SRMinfo {
public:
	SRMinfo* next_;
	
	int	sender_;

	/* Session messages */
	int	lsess_;			  /* # of last session msg received */
	int	sendTime_;		  /* Time sess. msg. # sent */
	int	recvTime_;		  /* Time sess. msg. # received */
	double	distance_;
	
	/* Data messages */
	int	ldata_;			  /* # of last data msg sent */
protected:
	char	*received_;		  /* Bit vector of msgs received */
	int	recvmax_;

#define	BITVEC_SIZE_DEFAULT	256	  // for (256 * 8) = 2K messages
	void resize(int id) {
		if (! received_) {
			received_ = new char[BITVEC_SIZE_DEFAULT];
			recvmax_ = BITVEC_SIZE_DEFAULT * sizeof(char);
			(void) memset(received_, '\0', BITVEC_SIZE_DEFAULT);
		}
		if (recvmax_ <= id) {
			int osize, nsize;
			nsize = osize = recvmax_;
			while (nsize <= id)
				nsize *= 2;
			osize /= sizeof(char);
			nsize /= sizeof(char);
			char* nvec = new char[nsize];
			(void) memcpy(nvec, received_, osize);
			(void) memset(nvec + osize, '\0', osize);
			delete[] received_;
			received_ = nvec;
			recvmax_ = nsize;
		}
	}	
		
public:
	SRMinfo(int sender) : next_(0), sender_(sender),
		lsess_(-1), sendTime_(0), recvTime_(0), distance_(1.0),
		ldata_(-1), received_(0), recvmax_(-1) { }
	~SRMinfo() { delete[] received_; }
	
	char ifReceived(int id) {
		assert(id >= 0);
		if (id >= recvmax_)
			resize(id);
		return (received_[id / 8] & (1 << (id % 8)));
	}
	char setReceived(int id) {
		int obit = ifReceived(id);
		received_[id / 8] |= (1 << (id % 8));
		return obit;
	}
	char resetReceived(int id) {
		int obit = ifReceived(id);
		received_[id / 8] &= ~(1 << (id % 8));
		return obit;
	}
};

#endif /* ns_srm_state_h */
