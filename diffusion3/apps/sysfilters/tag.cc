//
// tag.cc         : Tag Filter
// author         : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: tag.cc,v 1.2 2002/05/07 00:10:06 haldar Exp $
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

#include "tag.hh"

TagFilter *app;

#ifdef NS_DIFFUSION
static class TagFilterClass : public TclClass {
public:
  TagFilterClass() : TclClass("Application/DiffApp/TagFilter") {}
  TclObject * create(int argc, const char*const* argv) {
    return(new TagFilter());
  }
} class_tag_filter;

int TagFilter::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "start") == 0) {
      run();
      return (TCL_OK);
    }
  }
  return (DiffApp::command(argc, argv));
}
#endif // NS_DIFFUSION

void TagFilterReceive::recv(Message *msg, handle h)
{
  app->recv(msg, h);
}

void TagFilter::recv(Message *msg, handle h)
{
  if (h != filterHandle){
    fprintf(stderr, "Error: TagFilter::recv received message for handle %ld when subscribing to handle %ld !\n", h, filterHandle);
    return;
  }

  ProcessMessage(msg);

  ((DiffusionRouting *)dr_)->sendMessage(msg, h);
}

void TagFilter::ProcessMessage(Message *msg)
{
  char *original_route;
  char *new_route;
  int len, total_len;
  NRSimpleAttribute<char *> *route = NULL;

  // Can't do anything if node id unknown
  if (!id)
    return;

  route = RouteAttr.find(msg->msg_attr_vec_);
  if (!route){
    fprintf(stderr, "Error: Can't find the route attribute !\n");
    return;
  }

  original_route = route->getVal();
  len = strlen(original_route);

  if (len == 0){
    total_len = strlen(id);
    // Route is empty, need to allocate memory
    // for our id and the terminating '\0'
    new_route = new char[(total_len + 1)];
    strcpy(new_route, id);
    if (new_route[total_len] != '\0')
      fprintf(stderr, "Warning: String must end with NULL !\n");
  }
  else{
    // Route already exists. We need to allocate
    // memory for the current route + ':' + our
    // id + the terminating '\0'
    total_len = len + strlen(id) + 1;
    new_route = new char[(total_len + 1)];
    strcpy(new_route, original_route);
    new_route[len] = ':';
    strcpy(&new_route[len+1], id);
    if (new_route[total_len] != '\0'){
      fprintf(stderr, "Warning: String must end with NULL !\n");
    }
  }

  // Debug
  fprintf(stderr, "Tag Filter: Original route : %s\n", original_route);
  fprintf(stderr, "Tag Filter: New route : %s\n", new_route);

  route->setVal(new_route);

  // Free memory
  delete [] new_route;
}

handle TagFilter::setupFilter()
{
  NRAttrVec attrs;
  handle h;

  // Match all packets with a Route Attribute
  attrs.push_back(RouteAttr.make(NRAttribute::EQ_ANY, ""));

  h = ((DiffusionRouting *)dr_)->addFilter(&attrs, TAG_FILTER_PRIORITY, fcb);

  ClearAttrs(&attrs);
  return h;
}

void TagFilter::run()
{
#ifdef NS_DIFFUSION
  // Set up the filter
  filterHandle = setupFilter();
  fprintf(stderr, "Tag Filter subscribed to *, received handle %d\n", (int)filterHandle);
  fprintf(stderr, "Tag Filter initialized !\n");
#else
  // Doesn't do anything
  while (1){
    sleep(1000);
  }
#endif // NS_DIFFUSION
}

void TagFilter::getNodeId()
{
  fprintf(stderr, "Tag Filter: getNodeID function not yet implemented !\n");
  fprintf(stderr, "Tag Filter: Please set scadds_addr to the node id !\n");
  exit(-1);
  // Future implementation for the inter-module API
}

#ifdef NS_DIFFUSION
TagFilter::TagFilter()
#else
TagFilter::TagFilter(int argc, char **argv)
#endif // NS_DIFFUSION
{
  char *id_env = NULL;
  char buffer[BUFFER_SIZE];
  int flag;

  id = NULL;
  node_id = 0;

  // Create Diffusion Routing class
#ifndef NS_DIFFUSION
  parseCommandLine(argc, argv);
  dr_ = NR::createNR(diffusion_port_);
#endif // NS_DIFFUSION

  fcb = new TagFilterReceive(this);

  // Try to figure out the node ID
  id_env = getenv("scadds_addr");
  if (id_env){
    node_id = atoi(id_env);
    flag = snprintf(&buffer[0], BUFFER_SIZE, "%d", node_id);
    if (flag == -1 || flag == BUFFER_SIZE){
      fprintf(stderr, "Error: Buffer too small !\n");
      exit(-1);
    }
    id = strdup(&buffer[0]);
  }
  else{
    getNodeId();
  }
#ifndef NS_DIFFUSION
  // Set up the filter
  filterHandle = setupFilter();
  fprintf(stderr, "Tag Filter subscribed to *, received handle %ld\n", filterHandle);
  fprintf(stderr, "Tag Filter initialized !\n");
#endif // NS_DIFFUSION
}

#ifndef NS_DIFFUSION
int main(int argc, char **argv)
{
  TagFilter *app;

  // Initialize and run the Tag Filter
  app = new TagFilter(argc, argv);
  app->run();

  return 0;
}
#endif // NS_DIFFUSION
