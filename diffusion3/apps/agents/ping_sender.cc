//
// ping_sender.cc : Ping Server Main File
// author         : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: ping_sender.cc,v 1.3 2002/03/21 19:30:54 haldar Exp $
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

void sendDataTimer::expire(Event *e) {
  a_->send();
}

void PingSenderApp::send()
{
  struct timeval tmv;
  int retval;

  // Update time in the packet
  getTime(&tmv);
  lastEventTime->seconds = tmv.tv_sec;
  lastEventTime->useconds = tmv.tv_usec;

  // Send data probe
  diffPrint(DEBUG_ALWAYS, "Sending Data %d\n", last_seq_sent);
  retval = dr->send(pubHandle, &data_attr);

  // Update counter
  last_seq_sent++;
  counterAttr->setVal(last_seq_sent);

  // re-schedule the timer 
  sdt.resched(SEND_DATA_INTERVAL);
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

void MySenderReceive::recv(NRAttrVec *data, NR::handle my_handle)
{
  app->recv(data, my_handle);
}

void PingSenderApp::recv(NRAttrVec *data, NR::handle my_handle)
{
  NRSimpleAttribute<int> *nrclass = NULL;

  nrclass = NRClassAttr.find(data);

  if (!nrclass){
    diffPrint(DEBUG_ALWAYS, "Received a BAD packet !\n");
    return;
  }

  switch (nrclass->getVal()){

  case NRAttribute::INTEREST_CLASS:

    diffPrint(DEBUG_ALWAYS, "Received an Interest message !\n");
    break;

  case NRAttribute::DISINTEREST_CLASS:

    diffPrint(DEBUG_ALWAYS, "Received a Disinterest message !\n");
    break;

  default:

    diffPrint(DEBUG_ALWAYS, "Received an unknown message (%d)!\n", nrclass->getVal());
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

  handle h = dr->subscribe(&attrs, mr);

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

  handle h = dr->publish(&attrs);

  ClearAttrs(&attrs);

  return h;
}

void PingSenderApp::run()
{
  struct timeval tmv;
#ifdef INTERACTIVE
  fd_set FDS;
#endif // INTERATIVE

  // Setup publication and subscription
  subHandle = setupSubscription();
  pubHandle = setupPublication();

  // Create time attribute
  getTime(&tmv);
  lastEventTime = new EventTime;
  lastEventTime->seconds = tmv.tv_sec;
  lastEventTime->useconds = tmv.tv_usec;
  timeAttr = TimeAttr.make(NRAttribute::IS, (void *) &lastEventTime,
			   sizeof(EventTime));
  data_attr.push_back(timeAttr);

  // Change pointer to point to attribute's payload
  delete lastEventTime;
  lastEventTime = (EventTime *) timeAttr->getVal();

  // Create counter attribute
  counterAttr = AppCounterAttr.make(NRAttribute::IS, last_seq_sent);
  data_attr.push_back(counterAttr);

#ifndef NS_DIFFUSION
  // Main thread will send ping probes
  while(1){
#ifdef INTERACTIVE
    FD_SET(0, &FDS);
    fprintf(stdout, "Press <Enter> to send a ping probe...");
    fflush(NULL);
    select(1, &FDS, NULL, NULL, NULL);
    input = getc(stdin);
    if (input == 'c'){
      dr->unsubscribe(subHandle);
      exit(0);
    }
#else
    sleep(5);
#endif // INTERACTIVE

    // Update time in the packet
    getTime(&tmv);
    lastEventTime->seconds = tmv.tv_sec;
    lastEventTime->useconds = tmv.tv_usec;

    // Send data probe
    diffPrint(DEBUG_ALWAYS, "Sending Data %d\n", last_seq_sent);
    retval = dr->send(pubHandle, &data_attr);

    // Update counter
    last_seq_sent++;
    counterAttr->setVal(last_seq_sent);
  }
#else
  send();
#endif // NS_DIFFUSION
}

#ifdef NS_DIFFUSION
PingSenderApp::PingSenderApp() : sdt(this)
#else
PingSenderApp::PingSenderApp(int argc, char **argv)
#endif // NS_DIFFUSION
{
  last_seq_sent = 0;

  mr = new MySenderReceive;
  mr->app = this;

#ifndef NS_DIFFUSION
  ParseCommandLine(argc, argv);
  dr = NR::createNR(diffusion_port);
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
