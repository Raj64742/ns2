// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
//
// Time-stamp: <2000-09-15 12:52:53 haoboy>
//
// Copyright (c) 2000 by the University of Southern California
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


//agent.cc

#ifdef NS_DIFFUSION

#include "agent.hh"

NRSimpleAttributeFactory<char *> TargetAttr(NRAttribute::TARGET_KEY, NRAttribute::STRING_TYPE);
//  NRSimpleAttributeFactory<char *> AppStringAttr(APP_KEY2, NRAttribute::STRING_TYPE);
//  NRSimpleAttributeFactory<int> AppCounterAttr(APP_KEY3, NRAttribute::INT32_TYPE);
//  NRSimpleAttributeFactory<int> AppDummyAttr(APP_KEY1, NRAttribute::INT32_TYPE);
#endif // NS
