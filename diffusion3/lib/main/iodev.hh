// 
// iodev.hh      : Diffusion IO Devices Include File
// authors       : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: iodev.hh,v 1.7 2003/07/09 17:50:02 haldar Exp $
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

// This file defines the basic interface between diffusion device
// drivers and the Diffusion Core (or the Diffusion Routing Library
// API). It provides basic functionality to send and receive packets
// and also keeps track of input/output file descriptors that can be
// used on select().

#ifndef _IODEV_HH_
#define _IODEV_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <list>

#include "header.hh"
#include "tools.hh"

using namespace std;

class DiffusionIO {
public:
  DiffusionIO();

  virtual ~DiffusionIO(){
    // Nothing to do
  };

  virtual void addInFDS(fd_set *fds, int *max);
  virtual bool hasFD(int fd);
  virtual int checkInFDS(fd_set *fds);
  virtual DiffPacket recvPacket(int fd) = 0;
  virtual void sendPacket(DiffPacket p, int len, int dst) = 0;

protected:
  int num_out_descriptors_;
  int num_in_descriptors_;
  int max_in_descriptor_;
  list<int> in_fds_;
  list<int> out_fds_;
};

typedef list<DiffusionIO *> DeviceList;

#endif // !_IODEV_HH_
