/*
 * A possible optimizer for wireless simulation
 *
 * No guarantee for making wireless simulation better
 * Use it according to your scenario
 *
 * Ported from Sun's mobility code
 */

#ifndef __gridkeeper_h__
#define __gridkeeper_h__


#include "mobilenode.h"

#define min(a,b) (((a)>(b))?(b):(a))
#define max(a,b) (((a)<(b))?(b):(a))
#define aligngrid(a,b) (((a)==(b))?((b)-1):((a)))


class GridHandler : public Handler {
 public:
  GridHandler(MobileNode *mn);
  void handle(Event *);
 private:
  MobileNode * token_;
};

class GridKeeper : public TclObject {

public:
  GridKeeper();
  ~GridKeeper();
  int command(int argc, const char*const* argv);
  int get_neighbors(MobileNode *mn, MobileNode **output);


protected:
  int dim_x_;
  int dim_y_;
  MobileNode ***grid_;
  void new_moves(MobileNode *);
};

class MoveEvent : public Event {
public:
  MoveEvent() : enter_(0), leave_(0), grid_x_(-1), grid_y_(-1) {}
  MobileNode **enter_;	/* grid to enter */
  MobileNode **leave_;	/* grid to leave */
  int grid_x_;
  int grid_y_;
};



#endif //gridkeeper_h


