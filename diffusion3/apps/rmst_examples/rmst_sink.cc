//
// rmst_sink.cc    : RmstSink Class Methods
// authors         : Fred Stann
//
// Copyright (C) 2003 by the University of Southern California
// $Id: rmst_sink.cc,v 1.1 2003/07/09 17:45:48 haldar Exp $
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

#include "rmst_sink.hh"

void RmstSnkReceive::recv(NRAttrVec *data, NR::handle my_handle)
{
  NRSimpleAttribute<int> *rmst_id_attr = NULL;
  NRSimpleAttribute<void *> *data_buf_attr = NULL;
  timeval cur_time;
  int rmst_no;
  int size;
  void *blob_ptr;

  printf("RMST-SNK::recv callback got an NRAttrVec.\n");
  GetTime (&cur_time);
  printf("  time: sec = %d\n", (unsigned int) cur_time.tv_sec);

  rmst_id_attr = RmstIdAttr.find(data);
  data_buf_attr = RmstDataAttr.find(data);

  rmst_no = rmst_id_attr->getVal();
  blob_ptr = data_buf_attr->getVal();
  size = data_buf_attr->getLen();

  printf("  Got a blob, rmstId = %d, size = %d\n", rmst_no, size);
  snk_->recv((void *)blob_ptr, size);
}

void RmstSink::run() {
  rmst_handle_ = setupInterest();
  while (1){
      sleep (1000);
  }
}  

void RmstSink::recv(void *blob, int size) {
  int i;
  char *tmpPtr = (char*)blob;
  printf("  Sink received a large blob - size = %d\n", size);
  for(i=0; i<size; i+=50){
    printf("%s\n", &tmpPtr[i]);
  }
  no_rec_++;
  if(no_rec_ >= blobs_to_rec_){
    printf("  Sink unsubscribes\n");
    dr_->unsubscribe(rmst_handle_);
  }
}

handle RmstSink::setupInterest()
{
  NRAttrVec attrs;

  printf("RMST-SNK::subscribing to all PCM_SAMPLEs\n");

  attrs.push_back(NRClassAttr.make(NRAttribute::IS, NRAttribute::INTEREST_CLASS));
  attrs.push_back(RmstTsprtCtlAttr.make(NRAttribute::EQ, RMST_RESP));
  attrs.push_back(RmstTargetAttr.make(NRAttribute::EQ, "PCM_SAMPLE"));

  handle h = dr_->subscribe(&attrs, mr);

  ClearAttrs(&attrs);
  return h;
}

RmstSink::RmstSink(int argc, char **argv) : blobs_to_rec_(4) {
  mr = new RmstSnkReceive(this);  
  parseCommandLine(argc, argv);
  dr_ = NR::createNR(diffusion_port_);
  no_rec_ = 0;
}

int main(int argc, char **argv){
  RmstSink *app;

  app = new RmstSink(argc, argv);
  app->run();
  return 0;
}
