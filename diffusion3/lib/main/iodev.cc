// 
// iodev.cc      : Diffusion IO Devices
// authors       : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: iodev.cc,v 1.7 2003/07/09 17:50:02 haldar Exp $
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

#include "iodev.hh"

DiffusionIO::DiffusionIO()
{
  num_out_descriptors_ = 0;
  num_in_descriptors_ = 0;
  max_in_descriptor_ = 0;
}

void DiffusionIO::addInFDS(fd_set *fds, int *max)
{
  list<int>::iterator itr;

  for (itr = in_fds_.begin(); itr != in_fds_.end(); ++itr){
    FD_SET(*itr, fds);
  }

  if (max_in_descriptor_ > *max)
    *max = max_in_descriptor_;
}

int DiffusionIO::checkInFDS(fd_set *fds)
{
  list<int>::iterator itr;
  int fd;

  for (itr = in_fds_.begin(); itr != in_fds_.end(); ++itr){
    fd = *itr;
    if (FD_ISSET(fd, fds)){
      return (fd);
    }
  }
  return -1;
}

bool DiffusionIO::hasFD(int fd)
{
  list<int>::iterator itr;
  int device_fd;

  for (itr = in_fds_.begin(); itr != in_fds_.end(); ++itr){
    device_fd = *itr;
    if (device_fd == fd)
      return true;
  }
  return false;
}
