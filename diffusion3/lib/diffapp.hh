//
// diffapp.hh     : Base Class for Diffusion Apps and Filters
// author         : Fabio Silva and Padma Haldar
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: diffapp.hh,v 1.8 2003/07/09 17:50:00 haldar Exp $
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

// This file defines the DiffApp class, the base class for diffusion's
// applications and filters. In addition to provide the basic NR class
// pointer and other diffusion related variables, it also implements a
// basic command line parsing that can be used to set debug level,
// diffusion port and an optional configuration file (its name will be
// stored in the char *config_file_ variable for later processing by
// the application/filter. Also, the file includes everything
// necessary for creating a diffusion application/filter.

#ifndef _DIFFAPP_HH_
#define _DIFFAPP_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <stdlib.h>
#include <stdio.h>
#include "dr.hh"

#ifdef NS_DIFFUSION
#include "app.h"
class DiffApp : public Application {
#else
class DiffApp {
#endif // NS_DIFFUSION
public:
  virtual void run() = 0;
#ifdef NS_DIFFUSION
  int command(int argc, const char*const* argv);
#endif // NS_DIFFUSION
protected:
  NR *dr_;
  u_int16_t diffusion_port_;
  char *config_file_;

#ifndef NS_DIFFUSION
  void usage(char *s);
  void parseCommandLine(int argc, char **argv);
#endif // !NS_DIFFUSION
};

#endif // !_DIFFAPP_HH_
