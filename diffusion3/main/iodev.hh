// 
// iodev.hh      : Diffusion IO Devices Include File
// authors       : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: iodev.hh,v 1.7 2002/03/21 19:30:55 haldar Exp $
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

#ifndef iodev_hh
#define iodev_hh

#include <list>

#include "header.hh"
#include "tools.hh"

using namespace std;

class DiffusionIO {
public:
  DiffusionIO();
  virtual ~DiffusionIO() {
    //Empty
  };
  void addInFDS(fd_set *fds, int *max);
  int checkInFDS(fd_set *fds);
  virtual DiffPacket recvPacket(int fd) = 0;
  virtual void sendPacket(DiffPacket p, int len, int dst) = 0;

protected:
  int num_out_descriptors;
  int num_in_descriptors;
  int max_in_descriptor;
  list<int> in_fds;
  list<int> out_fds;
};

#endif // iodev_hh
