//
// agent.hh       : Agents Include File
// author         : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: agent.hh,v 1.1 2001/11/08 17:49:57 haldar Exp $
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

#ifndef DIFFUSION_AGENT
#define DIFFUSION_AGENT

#include <stdlib.h>
#include <stdio.h>
#include "../../lib/dr.hh"

class MyReceive : public NR::Callback {
  public:
  virtual void recv(NRAttrVec *data, NR::handle my_handle) {}
  
};

class MySrcReceive : public MyReceive {

public:
  void recv(NRAttrVec *data, NR::handle my_handle);
};

class MySnkReceive : public MyReceive {

public:
  void recv(NRAttrVec *data, NR::handle my_handle);
  int last_seq_recv;
  int num_msg_recv;
};

#define APP_KEY1 3500
#define APP_KEY2 3600
#define APP_KEY3 3601

extern NRSimpleAttributeFactory<char *> TargetAttr;
extern NRSimpleAttributeFactory<int> AppDummyAttr;
extern NRSimpleAttributeFactory<char *> AppStringAttr;
extern NRSimpleAttributeFactory<int> AppCounterAttr;


#endif //diff_agent







