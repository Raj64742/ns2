//
// gear_tools.cc  : GEAR Tools
// authors        : Yan Yu and Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// Copyright (C) 2000-2002 by the University of California
// $Id: gear_tools.cc,v 1.1 2003/07/08 18:06:44 haldar Exp $
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

#include "gear_tools.hh"

bool IsSameLocation(GeoLocation src, GeoLocation dst)
{
  if  ((src.longitude_ == dst.longitude_) &&
       (src.latitude_ == dst.latitude_))
    return true;
  return false;
}

double Distance(double long1, double lat1, double long2, double lat2)
{
  double distance;
 
  distance = sqrt((long1 - long2) * (long1 - long2) +
		  (lat1 - lat2) * (lat1 - lat2));

  return distance;
}

//subregion must clear to be the same as target_region...
int HeuristicValueTable::retrieveEntry(GeoLocation *dst)
{
  int i;
  bool once = true;

  if (first_ == -1)
    return FAIL;

  for (i = first_; (once || (i != next(last_))); i = next(i)){
    if (IsSameLocation(table_[i].dst_, *dst))
      return i;
    once = false;
  }
  return FAIL;
}

bool HeuristicValueTable::updateEntry(GeoLocation dst,
				      double heuristic_value)
{
  double old_heuristic_value;
  int index;

  if ((index = retrieveEntry(&dst)) == FAIL){
    addEntry(dst, heuristic_value);
    return true;
  }
  else{
    old_heuristic_value = table_[index].heuristic_value_;
    table_[index].heuristic_value_ = heuristic_value;
    if (ABS(heuristic_value - old_heuristic_value)
	>= GEO_HEURISTIC_VALUE_UPDATE_THRESHOLD)
      return true;
  }
  return false;
}

void HeuristicValueTable::addEntry(GeoLocation dst, double heuristic_value)
{
  int i;

  if (num_entries_ >= FORWARD_TABLE_SIZE){
    if (((last_ + 1) % FORWARD_TABLE_SIZE) != first_){
      DiffPrint(DEBUG_IMPORTANT, "GEO: last: %d, first: %d\n", last_, first_);
    }

    assert(((last_ + 1) % FORWARD_TABLE_SIZE) == first_);

    first_++;
    first_ = first_ % FORWARD_TABLE_SIZE;

    last_++;
    last_ = last_ % FORWARD_TABLE_SIZE;
    i = last_;

    table_[i].dst_ = dst;
    table_[i].heuristic_value_ = heuristic_value;
    return;
  }

  if (first_ == -1){
    first_ = 0;
    last_ = 0;
  }
  else{
    last_++;
    last_ = last_ % FORWARD_TABLE_SIZE;
  }
  i = last_;

  table_[i].dst_ = dst;
  table_[i].heuristic_value_ = heuristic_value;

  num_entries_++;
}

//subregion must clear to be the same as target_region...
int LearnedCostTable::retrieveEntry(int neighbor_id, GeoLocation *dst)
{
  int i;
  bool once = true;

  if (first_ == -1)
    return FAIL;

  for (i = first_; (once || (i != next(last_))); i = next(i)){
    if (IsSameLocation(table_[i].dst_, *dst) &&
	(neighbor_id == table_[i].node_id_))
      return i;
    once = false;
  }

  return FAIL;
}

void LearnedCostTable::updateEntry(int neighbor_id, GeoLocation dst,
				   double learned_cost)
{
  int index;

  if ((index = retrieveEntry(neighbor_id, &dst)) == FAIL)
    addEntry(neighbor_id, dst, learned_cost);
  else
    table_[index].learned_cost_value_ = learned_cost;
}

void LearnedCostTable::addEntry(int neighbor_id, GeoLocation dst,
				double learned_cost)
{
  int i;

  if (num_entries_ >= LEARNED_COST_TABLE_SIZE){
    if (((last_ + 1) % LEARNED_COST_TABLE_SIZE) != first_){
      DiffPrint(DEBUG_IMPORTANT,
		"GEO: LEARNED cost table ERROR: last = %d, first = %d\n",
		last_, first_);
    }

    assert(((last_ + 1) % LEARNED_COST_TABLE_SIZE) == first_);

    first_++;
    first_ = first_ % LEARNED_COST_TABLE_SIZE;

    last_++;
    last_ = last_ % LEARNED_COST_TABLE_SIZE;
    i = last_;

    table_[i].node_id_ = neighbor_id;
    table_[i].dst_ = dst;
    table_[i].learned_cost_value_ = learned_cost;
    return;
  }

  if (first_ == -1){
    first_ = 0;
    last_ = 0;
  }
  else{
    last_++;
    last_ = last_ % LEARNED_COST_TABLE_SIZE;
  }
  i = last_;

  table_[i].node_id_ = neighbor_id;
  table_[i].dst_ = dst;
  table_[i].learned_cost_value_ = learned_cost;

  num_entries_++;
}
