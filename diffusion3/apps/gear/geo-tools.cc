#include <math.h>
#include "geo-routing.hh"

bool same_location(GeoLocation src, GeoLocation dst)
{
  if  ((src.x == dst.x) && (src.y == dst.y))
    return true;
  return false;
}

double Distance(double x1, double y1, double x2, double y2)
{
  double distance;
  distance = sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));

  return distance;
}

//subregion must clear to be the same as target_region...
int H_value_table::RetrieveEntry(GeoLocation *dst)
{
  int i;
  bool once = true;

  if (first == -1)
    return FAIL;

  for (i = first; (once || (i != Next(last))); i = Next(i)){
    if (same_location(table[i].dst, *dst))
      return i;
    once = false;
  }
  return FAIL;
}

bool H_value_table::UpdateEntry(GeoLocation dst, double  h_val)
{
  double old_val;
  int index;

  if ((index = RetrieveEntry(&dst)) == FAIL){
    AddEntry(dst, h_val);
    return true;
  }
  else{
    old_val = table[index].h_value;
    table[index].h_value = h_val;
    if (ABS(h_val - old_val) >= GEO_H_VALUE_UPDATE_THRE)
      return true;
  }
  return false;
}

void H_value_table::AddEntry(GeoLocation dst, double  h_val)
{
  int i;

  if (num_entries >= FORWARD_TABLE_SIZE){
    if (((last + 1) % FORWARD_TABLE_SIZE) != first){
      diffPrint(DEBUG_IMPORTANT, "GEO: last = %d, first = %d\n", last, first);
    }

    assert(((last + 1) % FORWARD_TABLE_SIZE) == first);

    first++;
    first = first % FORWARD_TABLE_SIZE;

    last++;
    last = last % FORWARD_TABLE_SIZE;
    i = last;

    table[i].dst = dst;
    table[i].h_value = h_val;
    return;
  }

  if (first == -1){
    first = 0;
    last = 0;
  }
  else{
    last++;
    last = last % FORWARD_TABLE_SIZE;
  }
  i = last;

  table[i].dst = dst;
  table[i].h_value = h_val;

  num_entries++;
}

//subregion must clear to be the same as target_region...
int Learned_cost_table::RetrieveEntry(int neighbor_id, GeoLocation *dst)
{
  int i;
  bool once = true;

  if (first == -1)
    return FAIL;

  for (i = first; (once || (i != Next(last))); i = Next(i)){
    if (same_location(table[i].dst, *dst) && (neighbor_id == table[i].node_id))
      return i;
    once = false;
  }

  return FAIL;
}

void Learned_cost_table::UpdateEntry(int neighbor_id, GeoLocation dst, double l_cost)
{
  int index;

  if ((index = RetrieveEntry(neighbor_id, &dst)) == FAIL)
    AddEntry(neighbor_id, dst, l_cost);
  else
    table[index].l_cost_value = l_cost;
}

void Learned_cost_table::AddEntry(int neighbor_id, GeoLocation dst, double l_cost)
{
  int i;

  if (num_entries >= LEARNED_COST_TABLE_SIZE){
    if (((last + 1) % LEARNED_COST_TABLE_SIZE) != first){
      diffPrint(DEBUG_IMPORTANT,
		"GEO: LEARNED cost table ERROR: last = %d, first = %d\n", last, first);
    }

    assert(((last + 1) % LEARNED_COST_TABLE_SIZE) == first);

    first++;
    first = first % LEARNED_COST_TABLE_SIZE;

    last++;
    last = last % LEARNED_COST_TABLE_SIZE;
    i = last;

    table[i].node_id = neighbor_id;
    table[i].dst = dst;
    table[i].l_cost_value = l_cost;
    return;
  }

  if (first == -1){
    first = 0;
    last = 0;
  }
  else{
    last++;
    last = last % LEARNED_COST_TABLE_SIZE;
  }
  i = last;

  table[i].node_id = neighbor_id;
  table[i].dst = dst;
  table[i].l_cost_value = l_cost;

  num_entries++;
}
