//
// srcrt.hh       : Source Route Filter Include File
// author         : Fabio Silva
//
// Copyright (C) 2000-2002 by the Unversity of Southern California
// $Id: srcrt.hh,v 1.1 2003/07/08 18:08:11 haldar Exp $
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
#endif // NS_DIFFUSION

#include "diffapp.hh"

#define SRCRT_FILTER_PRIORITY 90

class SrcRtFilter;

class SrcRtFilterReceive : public FilterCallback {
public:
  SrcRtFilterReceive(SrcRtFilter *app) : app_(app) {};
  void recv(Message *msg, handle h);

  SrcRtFilter *app_;
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
  handle filter_handle_;

  // Receive Callback for the filter
  SrcRtFilterReceive *filter_callback_;

  // Filter interface
  handle setupFilter();

  // Message Processing
  Message * ProcessMessage(Message *msg);
};
#endif // !SRCRT_HH
