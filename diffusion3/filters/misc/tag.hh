//
// tag.hh         : Tag Filter Include File
// author         : Fabio Silva
//
// Copyright (C) 2000-2002 by the Unversity of Southern California
// $Id: tag.hh,v 1.1 2003/07/08 18:08:11 haldar Exp $
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

#ifndef TAG_HH
#define TAG_HH

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "diffapp.hh"

#define TAG_FILTER_PRIORITY 200
#define BUFFER_SIZE 64

class TagFilter;

class TagFilterReceive : public FilterCallback {
public:
  TagFilterReceive(TagFilter *app) : app_(app) {};
  void recv(Message *msg, handle h);

  TagFilter *app_;
};

class TagFilter : public DiffApp {
public:
#ifdef NS_DIFFUSION
  TagFilter();
  int command(int argc, const char*const* argv);
#else
  TagFilter(int argc, char **argv);
#endif // NS_DIFFUSION

  void run();
  void recv(Message *msg, handle h);

protected:
  // General Variables
  handle filter_handle_;
  char *id_;

  // Receive Callback for the filter
  TagFilterReceive *filter_callback_;

  // Filter interface
  handle setupFilter();
  void getNodeId();

  // Message Processing
  void ProcessMessage(Message *msg);
};
#endif // !TAG_HH
