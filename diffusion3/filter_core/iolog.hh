//
// iolog.hh      : IO Log Include File
// Authors       : Fabio Silva and Yutaka Mori
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: iolog.hh,v 1.1 2003/07/10 21:50:27 haldar Exp $
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

#ifndef _IOLOG_HH_
#define _IOLOG_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <netinet/in.h>
#include "main/iohook.hh"

class IOLog : public IOHook {
public:
  IOLog(int32_t id);
  DiffPacket recvPacket(int fd);
  void sendPacket(DiffPacket pkt, int len, int dst);

  int32_t node_id_;
};

#endif // !_IOLOG_HH_
