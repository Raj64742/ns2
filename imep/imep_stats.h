/* -*- c++ -*-
   imep_stats.h
   $Id: imep_stats.h,v 1.1 1999/08/03 04:12:38 yaxu Exp $

   stats collected by the IMEP code
   */

#ifndef _cmu_imep_imep_stats_h
#define _cmu_imep_imep_stats_h

struct ImepStat {

  int new_in_adjacency;
  int new_neighbor;
  int delete_neighbor1;
  int delete_neighbor2;
  int delete_neighbor3;

  int qry_objs_created;
  int upd_objs_created;
  int clr_objs_created;

  int qry_objs_recvd;
  int upd_objs_recvd;
  int clr_objs_recvd;

  int num_objects_created;
  int sum_response_list_sz;
  int num_object_pkts_created;
  int num_object_pkts_recvd;
  int num_recvd_from_queue;

  int num_rexmitable_pkts;
  int num_rexmitable_fully_acked;
  int num_rexmitable_retired;
  int sum_rexmitable_retired_response_sz;
  int num_rexmits;

  int num_holes_created;
  int num_holes_retired;
  int num_unexpected_acks;
  int num_reseqq_drops;

  int num_out_of_window_objs;
  int num_out_of_order_objs;
  int num_in_order_objs;
};

#endif
