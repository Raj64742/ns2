//
// ping_sender.cc : Ping Server Main File
// author         : Fabio Silva
//
// Copyright (C) 2000-2002 by the Unversity of Southern California
// $Id: ping_sender.cc,v 1.2 2002/05/07 00:09:38 haldar Exp $
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

#include "ping_sender.hh"
#include <unistd.h>

#ifdef NS_DIFFUSION
static class PingSenderAppClass : public TclClass {
public:
  PingSenderAppClass() : TclClass("Application/DiffApp/PingSender") {}
  TclObject* create(int , const char*const*) {
    return(new PingSenderApp());
  }
} class_ping_sender;

void PingSendDataTimer::expire(Event *e) {
  a_->send();
}

void PingSenderApp::send()
{
  struct timeval tmv;
  int retval;

  // Update time in the packet
  GetTime(&tmv);
  lastEventTime_->seconds_ = tmv.tv_sec;
  lastEventTime_->useconds_ = tmv.tv_usec;

  // Send data probe
  DiffPrint(DEBUG_ALWAYS, "Sending Data %d\n", last_seq_sent_);
  retval = dr_->send(pubHandle_, &data_attr_);

  // Update counter
  last_seq_sent_++;
  counterAttr_->setVal(last_seq_sent_);

  // re-schedule the timer 
  sdt_.resched(SEND_DATA_INTERVAL);
}

int PingSenderApp::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "publish") == 0) {
      run();
      return TCL_OK;
    }
  }
  return DiffApp::command(argc, argv);
}
#endif // NS_DIFFUSION

void PingSenderReceive::recv(NRAttrVec *data, NR::handle my_handle)
{
  app_->recv(data, my_handle);
}

void PingSenderApp::recv(NRAttrVec *data, NR::handle my_handle)
{
  NRSimpleAttribute<int> *nrclass = NULL;

  nrclass = NRClassAttr.find(data);

  if (!nrclass){
    DiffPrint(DEBUG_ALWAYS, "Received a BAD packet !\n");
    return;
  }

  switch (nrclass->getVal()){

  case NRAttribute::INTEREST_CLASS:

    DiffPrint(DEBUG_ALWAYS, "Received an Interest message !\n");
    break;

  case NRAttribute::DISINTEREST_CLASS:

    DiffPrint(DEBUG_ALWAYS, "Received a Disinterest message !\n");
    break;

  default:

    DiffPrint(DEBUG_ALWAYS, "Received an unknown message (%d)!\n", nrclass->getVal());
    break;

  }
}

handle PingSenderApp::setupSubscription()
{
  NRAttrVec attrs;

  attrs.push_back(NRClassAttr.make(NRAttribute::NE, NRAttribute::DATA_CLASS));
  attrs.push_back(NRScopeAttr.make(NRAttribute::IS, NRAttribute::NODE_LOCAL_SCOPE));
  attrs.push_back(TargetAttr.make(NRAttribute::EQ, "F117A"));
  attrs.push_back(LatitudeAttr.make(NRAttribute::IS, 60.00));
  attrs.push_back(LongitudeAttr.make(NRAttribute::IS, 54.00));

  handle h = dr_->subscribe(&attrs, mr_);

  ClearAttrs(&attrs);

  return h;
}

handle PingSenderApp::setupPublication()
{
  NRAttrVec attrs;

  attrs.push_back(NRClassAttr.make(NRAttribute::IS, NRAttribute::DATA_CLASS));
  attrs.push_back(LatitudeAttr.make(NRAttribute::IS, 60.00));
  attrs.push_back(LongitudeAttr.make(NRAttribute::IS, 54.00));
  attrs.push_back(TargetAttr.make(NRAttribute::IS, "F117A"));

  handle h = dr_->publish(&attrs);

  ClearAttrs(&attrs);

  return h;
}

void PingSenderApp::run()
{
  struct timeval tmv;
#ifndef NS_DIFFUSION
  int retval;
#endif // !NS_DIFFUSION

#ifdef INTERACTIVE
  char input;
  fd_set FDS;
#endif // INTERATIVE

  // Setup publication and subscription
  subHandle_ = setupSubscription();
  pubHandle_ = setupPublication();

  // Create time attribute
  GetTime(&tmv);
  lastEventTime_ = new EventTime;
  lastEventTime_->seconds_ = tmv.tv_sec;
  lastEventTime_->useconds_ = tmv.tv_usec;
  timeAttr_ = TimeAttr.make(NRAttribute::IS, (void *) &lastEventTime_,
			    sizeof(EventTime));
  data_attr_.push_back(timeAttr_);

  // Change pointer to point to attribute's payload
  delete lastEventTime_;
  lastEventTime_ = (EventTime *) timeAttr_->getVal();

  // Create counter attribute
  counterAttr_ = AppCounterAttr.make(NRAttribute::IS, last_seq_sent_);
  data_attr_.push_back(counterAttr_);

#ifndef NS_DIFFUSION
  // Main thread will send ping probes
  while(1){
#ifdef INTERACTIVE
    FD_SET(0, &FDS);
    fprintf(stdout, "Press <Enter> to send a ping probe...");
    fflush(NULL);
    select(1, &FDS, NULL, NULL, NULL);
    input = getc(stdin);
#else
    sleep(5);
#endif // INTERACTIVE

    // Update time in the packet
    GetTime(&tmv);
    lastEventTime_->seconds_ = tmv.tv_sec;
    lastEventTime_->useconds_ = tmv.tv_usec;

    // Send data probe
    DiffPrint(DEBUG_ALWAYS, "Sending Data %d\n", last_seq_sent_);
    retval = dr_->send(pubHandle_, &data_attr_);

    // Update counter
    last_seq_sent_++;
    counterAttr_->setVal(last_seq_sent_);
  }
#else
  send();
#endif // NS_DIFFUSION
}

#ifdef NS_DIFFUSION
PingSenderApp::PingSenderApp() : sdt_(this)
#else
PingSenderApp::PingSenderApp(int argc, char **argv)
#endif // NS_DIFFUSION
{
  last_seq_sent_ = 0;

  mr_ = new PingSenderReceive(this);

#ifndef NS_DIFFUSION
  parseCommandLine(argc, argv);
  dr_ = NR::createNR(diffusion_port_);
#endif // NS_DIFFUSION
}

#ifndef NS_DIFFUSION
int main(int argc, char **argv)
{
  PingSenderApp *app;

  app = new PingSenderApp(argc, argv);
  app->run();

  return 0;
}
#endif // NS_DIFFUSION
