/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or
 * derivative work. Xerox grants no other licenses expressed or
 * implied. The Xerox trade name should not be used in any advertising
 * without its written permission. 
 *
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * software. 
 */
#ifndef lint
static const char rcsid[] =
	"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/adc.cc,v 1.6 1998/12/22 23:11:24 breslau Exp $";
#endif

#include "adc.h"
#include <stdlib.h>

ADC::ADC() :bandwidth_(0), tchan_(0)
{
	bind_bw("bandwidth_",&bandwidth_);
	bind_bool("backoff_",&backoff_);
	bind("src_", &src_);
	bind("dst_", &dst_);
}

int ADC::command(int argc,const char*const*argv)
{
	
	Tcl& tcl = Tcl::instance();
	if (argc==2) {
		if (strcmp(argv[1],"start") ==0) {
			/* $adc start */
			est_[1]->start();
			return (TCL_OK);
		}
	} else if (argc==4) {
		if (strcmp(argv[1],"attach-measmod") == 0) {
			/* $adc attach-measmod $meas $cl */
			MeasureMod *meas_mod = (MeasureMod *)TclObject::lookup(argv[2]);
			if (meas_mod== 0) {
				tcl.resultf("no measuremod found");
				return(TCL_ERROR);
			}
			int cl=atoi(argv[3]);
			est_[cl]->setmeasmod(meas_mod);
			return(TCL_OK);
		} else if (strcmp(argv[1],"attach-est") == 0 ) {
			/* $adc attach-est $est $cl */
			Estimator *est_mod = (Estimator *)TclObject::lookup(argv[2]);
			if (est_mod== 0) {
				tcl.resultf("no estmod found");
				return(TCL_ERROR);
			}
			int cl=atoi(argv[3]);
			setest(cl,est_mod);
			return(TCL_OK);
		}
	}
	else if (argc == 3) {
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
			if (tchan_ == 0) {
				tcl.resultf("ADC: trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
			
		}
		if (strcmp(argv[1], "setbuf") == 0) {
			/* some sub classes actually do something here */
			return(TCL_OK);
		}


	}
	return (NsObject::command(argc,argv));
}




