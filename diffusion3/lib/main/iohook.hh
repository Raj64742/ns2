//
// iohook.hh     : IO Hook Include File
// Authors       : Fabio Silva and Yutaka Mori
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: iohook.hh,v 1.1 2003/07/10 21:46:47 haldar Exp $
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

#ifndef _IOHOOK_HH_
#define _IOHOOK_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <netinet/in.h>
#include "iodev.hh"

class IOHook : public DiffusionIO {
public:
  IOHook();
  void addInFDS(fd_set *fds, int *max);
  int checkInFDS(fd_set *fds);
  virtual DiffPacket recvPacket(int fd);
  virtual void sendPacket(DiffPacket pkt, int len, int dst);

  DeviceList in_devices_;
  DeviceList out_devices_;
  DeviceList local_out_devices_;
};

#endif // !_IOHOOK_HH_
