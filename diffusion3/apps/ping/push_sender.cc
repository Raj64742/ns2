//
// push_sender.cc : Ping Server Main File
// author         : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: push_sender.cc,v 1.2 2004/01/09 00:15:23 haldar Exp $
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

#include "push_sender.hh"
#include <unistd.h>

#ifdef NS_DIFFUSION
static class PushSenderAppClass : public TclClass {
public:
  PushSenderAppClass() : TclClass("Application/DiffApp/PushSender") {}
  TclObject* create(int , const char*const*) {
    return(new PushSenderApp());
  }
} class_ping_sender;

void PushSendDataTimer::expire(Event *e) {
  a_->send();
}

void PushSenderApp::send()
{
  struct timeval tmv;
  int retval;

  // Update time in the packet
  GetTime(&tmv);
  lastEventTime_->seconds_ = tmv.tv_sec;
  lastEventTime_->useconds_ = tmv.tv_usec;

  // Send data probe
  DiffPrint(DEBUG_ALWAYS, "Node%d: Sending Data %d\n", ((DiffusionRouting *)dr_)->getNodeId(), last_seq_sent_);
  retval = dr_->send(pubHandle_, &data_attr_);

  // Update counter
  last_seq_sent_++;
  counterAttr_->setVal(last_seq_sent_);

  // re-schedule the timer 
  sdt_.resched(SEND_DATA_INTERVAL);
}

int PushSenderApp::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "publish") == 0) {
      run();
      return TCL_OK;
    }
  }
  return DiffApp::command(argc, argv);
}
#endif // NS_DIFFUSION

handle PushSenderApp::setupPublication()
{
  NRAttrVec attrs;

  attrs.push_back(NRClassAttr.make(NRAttribute::IS, NRAttribute::DATA_CLASS));
  attrs.push_back(NRScopeAttr.make(NRAttribute::IS, NRAttribute::GLOBAL_SCOPE));
  attrs.push_back(LatitudeAttr.make(NRAttribute::LE, 70.00));
  attrs.push_back(LatitudeAttr.make(NRAttribute::GT, 50.00));
  attrs.push_back(LongitudeAttr.make(NRAttribute::LE, 85.00));
  attrs.push_back(LongitudeAttr.make(NRAttribute::GT, 50.00));
  attrs.push_back(TargetAttr.make(NRAttribute::IS, "F117A"));

  handle h = dr_->publish(&attrs);

  ClearAttrs(&attrs);

  return h;
}

void PushSenderApp::run()
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
PushSenderApp::PushSenderApp() : sdt_(this)
#else
PushSenderApp::PushSenderApp(int argc, char **argv)
#endif // NS_DIFFUSION
{
  last_seq_sent_ = 0;

#ifndef NS_DIFFUSION
  parseCommandLine(argc, argv);
  dr_ = NR::createNR(diffusion_port_);
#endif // NS_DIFFUSION
}

#ifndef NS_DIFFUSION
int main(int argc, char **argv)
{
  PushSenderApp *app;

  app = new PushSenderApp(argc, argv);
  app->run();

  return 0;
}
#endif // NS_DIFFUSION
