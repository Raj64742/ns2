
extern "C" {
#include <stdarg.h>
#include <float.h>
};

#include "sensor-query.h"
#include "landmark.h"
#include <random.h>

#define QUERY_PKT 4

#define CONST_INTERVAL 30

static class SensorQueryClass:public TclClass
{
  public:
  SensorQueryClass ():TclClass ("Agent/SensorQuery")
  {
  }
  TclObject *create (int, const char *const *)
  {
    return (new SensorQueryAgent ());
  }
} class_sensor_query;




void SensorQueryAgent::
trace (char *fmt,...)
{
  va_list ap; // Define a variable ap that will refer to each argument in turn

  if (!tracetarget_)
    return;

  // Initializes ap to first argument
  va_start (ap, fmt); 
  // Prints the elements in turn
  vsprintf (tracetarget_->buffer (), fmt, ap); 
  tracetarget_->dump ();
  // Does the necessary clean-up before returning
  va_end (ap); 
}




int 
SensorQueryAgent::command (int argc, const char *const *argv)
{
  if (argc == 2)
    {
      if (strcmp (argv[1], "start-queries") == 0)
        {
          startUp();
          return (TCL_OK);
        }
      if (strcmp (argv[1], "generate-query") == 0)
        {
          generate_query(-1,-1,-1);
          return (TCL_OK);
        }
    }
  else if (argc == 3)
    {
      if (strcasecmp (argv[1], "tracetarget") == 0)
        {
          TclObject *obj;
	  if ((obj = TclObject::lookup (argv[2])) == 0)
            {
              fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1],
                       argv[2]);
              return TCL_ERROR;
            }
          tracetarget_ = (Trace *) obj;
          return TCL_OK;
        }
      else if (strcasecmp (argv[1], "addr") == 0) {
        int temp;
        temp = Address::instance().str2addr(argv[2]);
        myaddr_ = temp;
        return TCL_OK;
      }
      else if (strcasecmp (argv[1], "attach-tag-dbase") == 0)
        {
          TclObject *obj;
          if ((obj = TclObject::lookup (argv[2])) == 0)
            {
              fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1],
                       argv[2]);
              return TCL_ERROR;
            }
          tag_dbase_ = (tags_database *) obj;
          return TCL_OK;
        }
    }
  else if (argc == 5) {
    if (strcmp (argv[1], "generate-query") == 0)
      {
	int p1 = atoi(argv[2]);
	int p2 = atoi(argv[3]);
	int p3 = atoi(argv[4]);
	generate_query(p1,p2,p3);
	return (TCL_OK);
      }
  }
  
  return (Agent::command (argc, argv));
}





void
SensorQueryAgent::startUp()
{
  Scheduler &s = Scheduler::instance();
  double sched_time = CONST_INTERVAL + Random::uniform(query_interval_);
  trace("Node %d: Scheduling query gen at %f from now %f",myaddr_,sched_time,s.clock());
  s.schedule(query_handler_, gen_query_event_, Random::uniform(query_interval_));
}



//void
//SensorQueryAgent::handle(Event *) {
//  Scheduler &s = Scheduler::instance();
//  generate_query();
//  double sched_time = Random::uniform(query_interval_);
//  trace("Node %d: Scheduling query gen at %f from now %f",myaddr_,sched_time,s.clock());
//  s.schedule(this, gen_query_event_, Random::uniform(query_interval_));
//}




void
SensorQueryAgent::generate_query(int p1, int p2, int p3)
{
  Packet *p = allocpkt();
  hdr_ip *iph = (hdr_ip *) p->access(off_ip_);
  hdr_cmn *hdrc = HDR_CMN(p);
  unsigned char *walk;
  Scheduler &s = Scheduler::instance();
  int obj_name;
  int next_hop_level = 0, num_src_hops = 0;
  int X = 65000, Y = 65000;
  int action = QUERY_PKT;

  // Need to ask our routing module to direct the packet
  hdrc->next_hop_ = myaddr_;
  hdrc->addr_type_ = NS_AF_INET;
  iph->dst_ = (myaddr_ << 8) | (ROUTER_PORT);
  iph->dport_ = ROUTER_PORT;
  iph->ttl_ = 300;   // since only 300 ids in source route in packet

  iph->src_ = myaddr_;
  iph->sport_ = 0;

  // LM agent checks if the source port is 0 to identify a query packet  
  // 2 bytes each for X and Y co-ords
  // 1 byte for level - used in next hop lookup
  // 4 bytes for object name; 2 byte for number of hops in src route
  // 4 bytes for origin_time
  // 90 bytes to store 30 source route ids and their levels (1 byte each)
  // (assuming max_levels = 15) 
  p->allocdata(105);
  walk = p->accessdata();

  (*walk++) = action & 0xFF;

  // X coord = 65000 initially
  (*walk++) = (X >> 8) & 0xFF;
  (*walk++) = X & 0xFF;

  // Y coord = 65000 initially
  (*walk++) = (Y >> 8) & 0xFF;
  (*walk++) = Y & 0xFF;

  // Indicates next hop level
  (*walk++) = next_hop_level & 0xFF;

  if(p1 != -1 && p2 != -1 && p3 != -1)
    obj_name = (int) ((p1 * pow(2,24)) + (p2 * pow(2,16)) + p3) ;
  else
    obj_name = tag_dbase_->get_random_tag();

  //  p1 = Random::integer(10);
  //  p2 = Random::integer(10);
  //  p3 = Random::integer(50);

  (*walk++) = (obj_name >> 24) & 0xFF;
  (*walk++) = (obj_name >> 16) & 0xFF;
  (*walk++) = (obj_name >> 8) & 0xFF;
  (*walk++) = (obj_name) & 0xFF;

  double now = Scheduler::instance().clock();
  if(now < 2000)
    trace("Node %d: Generating query for object %d.%d.%d at time %f",myaddr_,(obj_name >> 24) & 0xFF,(obj_name >> 16) & 0xFF, obj_name & 0xFFFF,now);
  

  int origin_time = (int) now;
  (*walk++) = (origin_time >> 24) & 0xFF;
  (*walk++) = (origin_time >> 16) & 0xFF;
  (*walk++) = (origin_time >> 8) & 0xFF;
  (*walk++) = (origin_time) & 0xFF;

  // Number of source route hops in packet (= 0); 2 bytes
  (*walk++) = (num_src_hops >> 8) & 0xFF;
  (*walk++) = (num_src_hops) & 0xFF;

  // Above fields will be 4 bytes each. 20 bytes for the IP header will be
  // added in LM agent. No source route hops on query creation.
  hdrc->size_ = 24; 
  hdrc->direction() = -1;

  s.schedule(target_,p,0);
  //  double sched_time = CONST_INTERVAL + Random::uniform(query_interval_);
  //  trace("Node %d: Scheduling query gen at %f from now %f",myaddr_,sched_time,s.clock());
  //  s.schedule(query_handler_, gen_query_event_, sched_time);
  //  s.schedule(this, gen_query_event_, Random::uniform(query_interval_));
}



void
SensorQueryAgent::recv(Packet *p, Handler *)
{
  unsigned char *walk = p->accessdata();
  int X = 0, Y = 0, obj_name = -1;

  ++walk;
  X = *walk++;
  X = (X << 8) | *walk++;

  Y = *walk++;
  Y = (Y << 8) | *walk++;
  
  ++walk;
  obj_name = *walk++;
  obj_name = (obj_name << 8) | *walk++;
  obj_name = (obj_name << 8) | *walk++;
  obj_name = (obj_name << 8) | *walk++;

  double now = Scheduler::instance().clock();
  if(now < 2000) 
    trace("Found object %d.%d.%d at (%d,%d) at time %f", (obj_name >> 24) & 0xFF, (obj_name >> 16) & 0xFF, obj_name & 0xFFFF,X,Y,now);

  Packet::free(p);
}



SensorQueryAgent::SensorQueryAgent() : Agent(PT_MESSAGE) { 
    query_interval_ = 120;
    gen_query_event_ = new Event;
    query_handler_ = new SensorQueryHandler(this);
}


