//
// log.hh         : Log Filter Include File
// author         : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: log.hh,v 1.2 2002/03/20 22:49:40 haldar Exp $
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

#ifndef LOG_HH
#define LOG_HH

#include "diffapp.hh"

#define LOG_FILTER_PRIORITY 16

class LogFilter;

class LogFilterReceive : public FilterCallback {
public:
  LogFilterReceive(LogFilter *_app) : app(_app)
  {};

  LogFilter *app;

  void recv(Message *msg, handle h);
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
  handle filterHandle;

  // Receive Callback for the filter
  LogFilterReceive *fcb;

  // Setup the filter
  handle setupFilter();

  // Message Processing
  void ProcessMessage(Message *msg);
};

#endif // LOG_HH
