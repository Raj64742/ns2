/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
// utilities.cc
//      Debugging routines.
// 
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/rap/utilities.cc,v 1.1 1999/05/14 18:12:22 polly Exp $

#include <stdarg.h>
#include "utilities.h"

//----------------------------------------------------------------------
// DebugEnable
//      Enable DEBUG messages.
//----------------------------------------------------------------------

FILE * DebugEnable(unsigned int nodeid)
{
  FILE *log;			// Logfile for debug messages
  char logFileName[NAME_MAX];

  sprintf(logFileName, "rap.%u.log", nodeid);
  log = fopen(logFileName, "w"); 
  assert(log != NULL);

  return log;
}
  
//----------------------------------------------------------------------
// Debug
//      Print a debug message if debugFlag is enabled. Like printf.
//----------------------------------------------------------------------

void Debug(int debugFlag, FILE *log, char *format, ...)
{
  if (debugFlag) 
    {
      va_list ap;
      va_start(ap, format);
      vfprintf(log, format, ap);
      va_end(ap);
      fflush(log);
    }
}
