//
// log.hh         : Log Filter Include File
// author         : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: log.hh,v 1.1 2003/07/08 18:08:10 haldar Exp $
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

#ifndef _LOG_HH_
#define _LOG_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "diffapp.hh"

#define LOG_FILTER_PRIORITY 210

class LogFilter;

class LogFilterReceive : public FilterCallback {
public:
  LogFilterReceive(LogFilter *app) : app_(app) {};
  void recv(Message *msg, handle h);

  LogFilter *app_;
};

class LogFilter : public DiffApp {
public:
#ifdef NS_DIFFUSION
  LogFilter();
  int command(int argc, const char*const* argv);
#else
  LogFilter(int argc, char **argv);
#endif // NS_DIFFUSION

  void run();
  void recv(Message *msg, handle h);

protected:
  // General Variables/Functions
  handle filter_handle_;

  // Receive Callback for the filter
  LogFilterReceive *filter_callback_;

  // Setup the filter
  handle setupFilter();

  // Message Processing
  void ProcessMessage(Message *msg);
};

#endif // !_LOG_HH_
