// diff-src.cc : diffusion application for a source node
// adapted from
// agent2.cc      : Test agent #2 Main File
// author         : Fabio Silva
//
// Time-stamp: <2000-09-15 12:53:32 haoboy>
//
// Copyright (c) 2000 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
// diff-src.cc

#ifdef NS_DIFFUSION

#include "diff-src.h"
#include "attr.h"
#include "diffagent.h"
#include <unistd.h>


//MyReceive *mr;

//void DiffusionSource::createAttrTemplates() {

//  NRSimpleAttributeFactory<char *> TargetAttr(NRAttribute::TARGET_KEY, NRAttribute::STRING_TYPE);
//  NRSimpleAttributeFactory<char *> AppStringAttr(APP_KEY2, NRAttribute::STRING_TYPE);
//  NRSimpleAttributeFactory<int> AppCounterAttr(APP_KEY3, NRAttribute::INT32_TYPE);
//  NRSimpleAttributeFactory<int> AppDummyAttr(APP_KEY1, NRAttribute::INT32_TYPE);
//}


void MySrcReceive::recv(NRAttrVec *data, NR::handle my_handle)
{
  NRSimpleAttribute<int> *nrclass = NULL;

  nrclass = NRClassAttr.find(data);

  if (!nrclass){
    printf("SRC_APP:Received a BAD packet !\n");
    return;
  }

  switch (nrclass->getVal()){

  case NRAttribute::INTEREST_CLASS:

    printf("Received an Interest message !\n");
    break;

  case NRAttribute::DISINTEREST_CLASS:

    printf("Received a Disinterest message !\n");
    break;

  default:

    printf("Received an unknown message (%d)!\n", nrclass->getVal());
    break;

  }
}

#ifdef NS_DIFFUSION
static class DiffusionSourceClass : public TclClass {
public:
  DiffusionSourceClass() : TclClass("Application/DiffApp/PingSource") {}
  TclObject* create(int , const char*const* ) {
    return(new DiffusionSource());
  }
} class_diffusion_source;

void sendDataTimer::expire(Event *e) {
  a_->send();
}

DiffusionSource::DiffusionSource() : counter(0), sdt(this) {
  mr = new MySrcReceive;
}


void DiffusionSource::start() {
  subscribeHandle = setupInterest(dr);
  sendHandle = setupPublication(dr);

  data_attr.push_back(AppStringAttr.make(NRAttribute::IS, "Sensing is working !\0"));
  data_attr.push_back(AppDummyAttr.make(NRAttribute::EQ, 2001));
  counterAttr = AppCounterAttr.make(NRAttribute::IS, counter);
  data_attr.push_back(counterAttr);
  
  send();
}

void DiffusionSource::send() {
  printf("SRC_APP:Sending Data %d\n", counter);
  int retval = dr->send(sendHandle, &data_attr);
  
  counter++;
  
  counterAttr->setVal(counter);
  
  // re-schedule the timer 
  sdt.resched(SEND_DATA_INTERVAL);

}


int DiffusionSource::command(int argc, const char*const* argv) {
  Tcl& tcl = Tcl::instance();
  if (argc == 2) {
    if (strcmp(argv[1], "publish") == 0) {
      start();
      return TCL_OK;
    }
    if (strcmp(argv[1], "unpublish") == 0) {
      dr->unsubscribe(subscribeHandle);
      //exit(0);
    }
  }
  //if (argc == 3) {
  // if (strcmp(argv[1], "dr") == 0) {
  //  DiffAppAgent *agent;
  //  agent = (DiffAppAgent *)TclObject::lookup(argv[2]);
  //  dr = agent->dr();
  //  return TCL_OK;
  //}
  //}
  return DiffApp::command(argc, argv);
}
#endif


handle DiffusionSource::setupInterest(NR *dr)
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

handle DiffusionSource::setupPublication(NR *dr)
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

#ifdef SCADDS
void usage()
{
  fprintf(stderr, "Usage: agent [-p port] [-h]\n\n");
  fprintf(stderr, "\t-p - Uses port 'port' to talk to diffusion\n");
  fprintf(stderr, "\t-h - Prints this information\n");
  fprintf(stderr, "\n");
  exit(0);
}

int main(int argc, char **argv)
{
  NR *dr;
  int retval;
  int counter = 0;
  NRAttrVec data_attr;
  NRSimpleAttribute<int> *counterAttr;
  handle sendHandle;
  handle subscribeHandle;
  char input;
  int opt;
  u_int16_t diffusion_port = DEFAULT_DIFFUSION_PORT;

  // Parse command line options
  opterr = 0;

  while (1){
    opt = getopt(argc, argv, "hp:");
    switch (opt){

    case 'p':

      diffusion_port = (u_int16_t) atoi(optarg);
      if ((diffusion_port < 1024) || (diffusion_port >= 65535)){
	fprintf(stderr, "Error: Diffusion port must be between 1024 and 65535 !\n");
	exit(-1);
      }

      break;

    case 'h':

      usage();

      break;

    case '?':

      fprintf(stderr, "Error: %c isn't a valid option or its parameter is missing !\n", optopt);
      usage();

      break;

    case ':':

      fprintf(stderr, "Parameter missing !\n");
      usage();

      break;

    }

    if (opt == -1)
      break;
  }

  dr = NR::createNR(diffusion_port);
  mr = new MyReceive;

  subscribeHandle = setupInterest(dr);
  sendHandle = setupPublication(dr);

  data_attr.push_back(AppStringAttr.make(NRAttribute::IS, "Sensing is working !\0"));
  data_attr.push_back(AppDummyAttr.make(NRAttribute::EQ, 2001));
  counterAttr = AppCounterAttr.make(NRAttribute::IS, counter);
  data_attr.push_back(counterAttr);

  // Main thread will send stuff
  while(1){
#ifdef INTERACTIVE
    printf("Now what ? ");
    input = getc(stdin);
    printf("\n");
    if (input == 'c'){
      dr->unsubscribe(subscribeHandle);
      exit(0);
    }
#else
    sleep(5);
#endif
    printf("Sending Data %d\n", counter);
    retval = dr->send(sendHandle, &data_attr);

    counter++;

    counterAttr->setVal(counter);
  }

  return 0;
}

#endif //scadds

#endif //NS
