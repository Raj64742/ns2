// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
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
//
// diff-snk.cc 

#ifdef NS_DIFFUSION

#include "diff-snk.h"
#include "diffagent.h"
//#include "attr.h"


void MySnkReceive::recv(NRAttrVec *data, NR::handle my_handle)
{
  
  NRSimpleAttribute<int> *counterAttr = NULL;

  counterAttr = AppCounterAttr.find(data);

  if (!counterAttr){
    printf("SINK_APP:Received a BAD packet !\n");
    printAttrs(data);
    return;
  }

  if (last_seq_recv >= 0){
    if (counterAttr->getVal() < last_seq_recv){
      last_seq_recv = -1;
      printf("SINK_APP:Received data %d !\n", counterAttr->getVal());
      // Multiple sources detected, disabling statistics
    }
    else{
      last_seq_recv = counterAttr->getVal();
      num_msg_recv++;
      printf("SINK_APP:Received data: %d, %% messages received: %f !\n",
	     last_seq_recv,
	     (float) ((num_msg_recv * 100.00) / (last_seq_recv + 1)));
    }
  }
  else{
    printf("SINK_APP:Received data %d !\n", counterAttr->getVal());
  }
}


#ifdef NS_DIFFUSION
static class DiffusionSinkClass : public TclClass {
public:
    DiffusionSinkClass() : TclClass("Application/DiffApp/PingSink") {}
    TclObject* create(int , const char*const* ) {
	    return(new DiffusionSink());
    }
} class_diffusion_sink;


int DiffusionSink::command(int argc, const char*const* argv) {
  Tcl& tcl = Tcl::instance();
  if (argc == 2) {
    if (strcmp(argv[1], "subscribe") == 0) {
      start();
      return TCL_OK;
    }
    if (strcmp(argv[1], "unsubscribe") == 0) {
      dr->unsubscribe(subHandle);
      //exit(0);
    }
  }
  //if (argc == 3) {
  //if (strcmp(argv[1], "dr") == 0) {
  //  DiffAppAgent *agent;
  //  agent = (DiffAppAgent *)TclObject::lookup(argv[2]);
  //  dr = agent->dr();
  //  return TCL_OK;
  //}
  //}
  return DiffApp::command(argc, argv);
}

void DiffusionSink::start() {
  mr->last_seq_recv = 0;
  mr->num_msg_recv = 0;
  subHandle = setupInterest(dr);
}  


#endif

DiffusionSink::DiffusionSink() {
  //declare attr factories
  //NRSimpleAttributeFactory<char *> TargetAttr(NRAttribute::TARGET_KEY, NRAttribute::STRING_TYPE);
  //NRSimpleAttributeFactory<int> AppDummyAttr(APP_KEY1, NRAttribute::INT32_TYPE);
  //NRSimpleAttributeFactory<int> AppCounterAttr(APP_KEY3, NRAttribute::INT32_TYPE);

  mr = new MySnkReceive;  
  

}

handle DiffusionSink::setupInterest(NR *dr)
{
  NRAttrVec attrs;

  attrs.push_back(NRClassAttr.make(NRAttribute::IS, NRAttribute::INTEREST_CLASS));
  attrs.push_back(LatitudeAttr.make(NRAttribute::GT, 54.78));
  attrs.push_back(LongitudeAttr.make(NRAttribute::LE, 87.32));
  attrs.push_back(TargetAttr.make(NRAttribute::IS, "F117A"));
  attrs.push_back(AppDummyAttr.make(NRAttribute::IS, 2001));

  handle h = dr->subscribe(&attrs, mr);

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
  handle subHandle;
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

  subHandle = setupInterest(dr);

  last_seq_recv = 0;
  num_msg_recv = 0;

  while (1){
    char input;

    printf("Now what ? ");
    input = getc(stdin);

    if (input == 'c'){
      dr->unsubscribe(subHandle);
      exit(0);
    }
  }

  return 0;
}
#endif //scadds

#endif// NS





