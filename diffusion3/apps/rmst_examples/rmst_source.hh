//
// rmst_source.hh  : RmstSource Class
// authors         : Fred Stann
//
// Copyright (C) 2003 by the University of Southern California
// $Id: rmst_source.hh,v 1.1 2003/07/09 17:45:48 haldar Exp $
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

#ifndef RMST_SRC
#define RMST_SRC

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif //HAVE_CONFIG_H

#include "diffapp.hh"

#define PAYLOAD_SIZE 50
class RmstSource;

class RmstSrcReceive : public NR::Callback {
public:
  RmstSrcReceive(RmstSource *rmstSrc) {src_ = rmstSrc; }
  void recv(NRAttrVec *data, NR::handle my_handle);
protected:
  RmstSource *src_;
};

class RmstSource : public DiffApp {
public:
  RmstSource(int argc, char **argv);
  virtual ~RmstSource(){};

  handle setupRmstInterest();
  handle setupRmstPublication();

  void run();
  void recv(Message *msg, handle h) { } 
  NR* getDr() {return dr_;} 
  int blobs_to_send_;
  char* createBlob(int id);
  void sendBlob();
  handle send_handle_;
  int num_subscriptions_;
 private:
  RmstSrcReceive *mr;
  handle subscribe_handle_;
  int ck_val_;
};

#endif //RMST_SRC
