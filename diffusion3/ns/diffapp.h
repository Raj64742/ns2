
#ifndef NS_DIFF_APP
#define NS_DIFF_APP

#include "app.h"

//base class for all diffusion related applications
#ifdef NS_DIFFUSION
class DiffApp : public Application
#else
class DiffApp 
#endif //ns
{
public:
  virtual void run() = 0;
  virtual void recv(Message *msg, handle h) = 0;
protected:
  NR *dr;
};




#endif //NS_DIFF_APP

