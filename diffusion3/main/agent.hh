//
// agent.hh        : Agent Definitions
// authors         : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: agent.hh,v 1.3 2002/03/20 22:49:40 haldar Exp $
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

#ifndef AGENT_HH
#define AGENT_HH

#include "tools.hh"

class Agent_Entry;

typedef list<Agent_Entry *> AgentList;

class Agent_Entry {
public:
  int32_t node_addr;
  u_int16_t port;
  struct timeval tv;
  
  Agent_Entry()
  {
    node_addr = 0;
    port = 0;
    getTime(&tv);
  };

  ~Agent_Entry() {};
};

#endif
