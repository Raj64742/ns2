//
// diffapp.hh     : Base Class for Diffusion Apps and Filters
// author         : Fabio Silva and Padma Haldar
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: diffapp.hh,v 1.2 2002/03/20 22:49:40 haldar Exp $
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

#ifndef DIFFAPP_HH
#define DIFFAPP_HH

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
  NR *dr;
  u_int16_t diffusion_port;
  char *config_file;

#ifndef NS_DIFFUSION
  void usage(char *s);
  void ParseCommandLine(int argc, char **argv);
#endif // NS_DIFFUSION
};

#endif // DIFFAPP_HH
