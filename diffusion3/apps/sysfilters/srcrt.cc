//
// srcrt.cc       : Source Route Filter
// author         : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: srcrt.cc,v 1.2 2002/05/07 00:10:06 haldar Exp $
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

#include "srcrt.hh"
#ifdef NS_DIFFUSION
class DiffAppAgent;
#endif //NS

#ifdef NS_DIFFUSION
static class SourceRouteFilterClass : public TclClass {
public:
  SourceRouteFilterClass() : TclClass("Application/DiffApp/SourceRouteFilter") {}
  TclObject* create(int argc, const char*const* argv) {
    return(new SrcRtFilter());
  }
} class_source_route_filter;

int SrcRtFilter::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "start") == 0) {
      run();
      return (TCL_OK);
    }
  }
  return (DiffApp::command(argc, argv));
}
#endif // NS_DIFFUSION

void SrcrtFilterReceive::recv(Message *msg, handle h)
{
  app->recv(msg, h);
}

void SrcRtFilter::recv(Message *msg, handle h)
{
  Message *return_msg = NULL;

  if (h != filterHandle){
    fprintf(stderr, "Error: Received a message for handle %ld when subscribing to handle %ld !\n", h, filterHandle);
    return;
  }

  return_msg = ProcessMessage(msg);

  if (return_msg){
    ((DiffusionRouting *)dr_)->sendMessage(msg, h);

    delete msg;
  }
}

Message * SrcRtFilter::ProcessMessage(Message *msg)
{
  char *original_route, *new_route, *p;
  int len;
  int32_t next_hop;
  NRSimpleAttribute<char *> *route = NULL;

  route = SourceRouteAttr.find(msg->msg_attr_vec_);
  if (!route){
    fprintf(stderr, "Error: Can't find the route attribute !\n");
    return msg;
  }

  original_route = route->getVal();
  len = strlen(original_route);

  // Check if we are the last hop
  if (len == 0)
    return msg;

  // Get the next hop
  next_hop = atoi(original_route);

  // Remove last hop from source route
  p = strstr(original_route, ":");
  if (!p){
    // There's just one more hop
    new_route = new char[1];
    new_route[0] = '\0';
  }
  else{
    p++;
    len = strlen(p);
    new_route = new char[(len + 1)];
    strncpy(new_route, p, (len + 1));
    if (new_route[len] != '\0')
      fprintf(stderr, "Warning: String must end with NULL !\n");
  }

  route->setVal(new_route);

  // Free memory
  delete [] new_route;

  // Send the packet to the next hop
  msg->next_hop_ = next_hop;
  ((DiffusionRouting *)dr_)->sendMessage(msg, filterHandle);

  delete msg;

  return NULL;
}

handle SrcRtFilter::setupFilter()
{
  NRAttrVec attrs;
  handle h;

  // Match all packets with a SourceRoute Attribute
  attrs.push_back(SourceRouteAttr.make(NRAttribute::EQ_ANY, ""));

  h = ((DiffusionRouting *)dr_)->addFilter(&attrs, SRCRT_FILTER_PRIORITY, fcb);

  ClearAttrs(&attrs);
  return h;
}

void SrcRtFilter::run()
{
#ifdef NS_DIFFUSION
  filterHandle = setupFilter();
  fprintf(stderr, "SrcRtFilter filter received handle %ld\n", filterHandle);
  fprintf(stderr, "SrcRtFilter filter initialized !\n");
#else
  // Doesn't do anything
  while (1){
    sleep(1000);
  }
#endif // NS_DIFFUSION
}

#ifdef NS_DIFFUSION
SrcRtFilter::SrcRtFilter()
#else
SrcRtFilter::SrcRtFilter(int argc, char **argv)
#endif //NS
{

#ifdef NS_DIFFUSION
  DiffAppAgent *agent;
#else
  // Create Diffusion Routing class
  parseCommandLine(argc, argv);
  dr_ = NR::createNR(diffusion_port_);
#endif // NS_DIFFUSION
      
  fcb = new SrcrtFilterReceive(this);

#ifndef NS_DIFFUSION
  // Set up the filter
  filterHandle = setupFilter();
  fprintf(stderr, "SrcRtFilter filter received handle %ld\n", filterHandle);
  fprintf(stderr, "SrcRtFilter filter initialized !\n");
#endif // NS_DIFFUSION
}

#ifndef NS_DIFFUSION
int main(int argc, char **argv)
{
  SrcRtFilter *app;

  // Initialize and run the Source Route Filter
  app = new SrcRtFilter(argc, argv);
  app->run();

  return 0;
}
#endif // NS_DIFFUSION
