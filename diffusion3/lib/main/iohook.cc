//
// iohook.cc     : IO Hook for Diffusion
// Authors       : Fabio Silva and Yutaka Mori
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: iohook.cc,v 1.1 2003/07/10 21:46:47 haldar Exp $
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

#include <stdio.h>
#include <stdlib.h>

#include "iohook.hh"

IOHook::IOHook() : DiffusionIO()
{
}

DiffPacket IOHook::recvPacket(int fd)
{
  DeviceList::iterator device_itr;
  DiffPacket incoming_packet = NULL;

  for (device_itr = in_devices_.begin();
       device_itr != in_devices_.end(); ++device_itr){
    if ((*device_itr)->hasFD(fd)){
      // We now have the correct device
      incoming_packet = (*device_itr)->recvPacket(fd);
      break;
    }
  }

  // Make sure packet was received
  if (incoming_packet == NULL){
    DiffPrint(DEBUG_ALWAYS, "Cannot read packet from driver !\n");
    return NULL;
  }

  return incoming_packet;
}

void IOHook::sendPacket(DiffPacket pkt, int len, int dst)
{
  DeviceList::iterator device_itr;

  for (device_itr = out_devices_.begin();
       device_itr != out_devices_.end(); ++device_itr){
    (*device_itr)->sendPacket(pkt, len, dst);
  }
}

void IOHook::addInFDS(fd_set *fds, int *max)
{
  DeviceList::iterator device_itr;

  for (device_itr = in_devices_.begin();
       device_itr != in_devices_.end(); ++device_itr){
    (*device_itr)->addInFDS(fds, max);
  }
}

int IOHook::checkInFDS(fd_set *fds)
{
  DeviceList::iterator device_itr;
  int fd;

  for (device_itr = in_devices_.begin();
       device_itr != in_devices_.end(); ++device_itr){
    fd = (*device_itr)->checkInFDS(fds);
    if (fd != 0)
      return fd;
  }

  return 0;
}

