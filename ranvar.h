/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *  
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or derivative
 * work. Xerox grants no other licenses expressed or implied. The Xerox trade
 * name should not be used in any advertising without its written permission.
 *  
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *  
 * These notices must be retained in any copies of any part of this software.
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/ranvar.h,v 1.3 1997/08/10 07:49:49 mccanne Exp $ (Xerox)
 */

#ifndef ns_ranvar_h
#define ns_ranvar_h

#include "tclcl.h"
#include "random.h"

class RandomVariable : public TclObject {
 public:
        virtual double value() = 0;
	int command(int argc, const char*const* argv);
 protected:
};

#endif
