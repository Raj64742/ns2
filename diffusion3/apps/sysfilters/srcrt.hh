//
// srcrt.hh       : Source Route Filter Include File
// author         : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: srcrt.hh,v 1.2 2002/05/07 00:10:06 haldar Exp $
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//
//

#ifndef SRCRT_HH
#define SRCRT_HH

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef NS_DIFFUSION
#include "diffagent.h"
#endif //NS
#include "diffapp.hh"

#define SRCRT_FILTER_PRIORITY 15

class SrcRtFilter;

class SrcrtFilterReceive : public FilterCallback {
public:
  SrcRtFilter *app;

  SrcrtFilterReceive(SrcRtFilter *_app) : app(_app) {};
  void recv(Message *msg, handle h);
};

class SrcRtFilter : public DiffApp {
public:
#ifdef NS_DIFFUSION
  SrcRtFilter();
  int command(int argc, const char*const* argv);
#else
  SrcRtFilter(int argc, char **argv);
#endif // NS_DIFFUSION

  void run();
  void recv(Message *msg, handle h);

protected:
  // General Variables
  handle filterHandle;

  // Receive Callback for the filter
  SrcrtFilterReceive *fcb;

  // Filter interface
  handle setupFilter();

  // Message Processing
  Message * ProcessMessage(Message *msg);
};
#endif // SRCRT_HH
