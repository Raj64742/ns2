//
// rmst_sink.hh    : RmstSink Class
// authors         : Fred Stann
//
// Copyright (C) 2003 by the University of Southern California
// $Id: rmst_sink.hh,v 1.2 2003/07/10 21:18:56 haldar Exp $
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

#ifndef RMST_SNK
#define RMST_SNK

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diffapp.hh"

class RmstSink;

class RmstSnkReceive : public NR::Callback {
public:
  RmstSnkReceive(RmstSink *rmstSnk) { snk_ = rmstSnk; }
  void recv(NRAttrVec *data, NR::handle my_handle);
protected:
  RmstSink *snk_; 
};

class RmstSink : public DiffApp {
public:
#ifdef NS_DIFFUSION
  RmstSink();
  int command(int argc, const char*const* argv);
#else
  RmstSink(int argc, char **argv);
#endif // NS_DIFFUSION

  virtual ~RmstSink(){};
  handle setupInterest();
  void run();
  void createAttrTemplates();
  void recv(void *blob, int size);
  NR* getDr() {return dr_;}
  handle rmst_handle_;         // PCM Sample interest
private:
  RmstSnkReceive *mr;
  int blobs_to_rec_;
  int no_rec_;
};

#endif // RMST_SNK
