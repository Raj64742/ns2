// simple worm model based on message passing
// 

#include "worm.h"
#include "random.h"
#include "math.h"

// timer for sending probes
void ProbingTimer::expire(Event*) {
  t_->timeout();
}

// base class for worm: host is invulnerable by default
static class WormAppClass : public TclClass {
public:
	WormAppClass() : TclClass("Application/Worm") {}
	TclObject* create(int, const char*const*) {
		return (new WormApp());
	}
} class_app_worm;

WormApp::WormApp() : Application() {
}

void WormApp::process_data(int nbytes, AppData* data) {
  recv(nbytes);
}

void WormApp::recv(int nbytes) {
  printf("Node %d is NOT vulnerable!\n", my_addr_);
}

void WormApp::timeout() {
}

int WormApp::command(int argc, const char*const* argv) {
  Tcl& tcl = Tcl::instance();

  if (argc == 3) {
    if (strcmp(argv[1], "attach-agent") == 0) {
      agent_ = (Agent*) TclObject::lookup(argv[2]);
      if (agent_ == 0) {
	tcl.resultf("no such agent %s", argv[2]);
	return(TCL_ERROR);
      }
      agent_->attachApp(this);
      my_addr_ = agent_->addr();
      //printf("%d\n", my_addr_);
      return(TCL_OK);
    }
  }

  return(Application::command(argc, argv));
}

// class to model vulnerable hosts in detailed network
static class DnhWormAppClass : public TclClass {
public:
	DnhWormAppClass() : TclClass("Application/Worm/Dnh") {}
	TclObject* create(int, const char*const*) {
		return (new DnhWormApp());
	}
} class_app_worm_dnh;

DnhWormApp::DnhWormApp() : WormApp() {
  infected_ = 0;
  timer_ = NULL;
  default_gw_ = 0;
  addr_low_ = addr_high_ = my_addr_;

  // get probing rate from configuration
  bind("ScanRate", &p_rate_);

  // get probing port from configuration
  bind("ScanPort", &p_port_);

  // get probing port from configuration
  bind("ScanPacketSize", &p_size_);
}

void DnhWormApp::recv(int nbytes) {
  if (infected_) {
    printf("Node %d is infected already...\n", my_addr_);
  } else {
    printf("Node %d is compromised...:(\n", my_addr_);

    // start to probe other hosts
    probe();
  }
}

void DnhWormApp::timeout() {
  timer_->resched(p_inv_);
  send_probe();
}

void DnhWormApp::probe() {
  infected_ = 1;
  total_addr_ = (int)pow(2, 32) - 1;

  if (p_rate_) {
    p_inv_ = 1.0 / p_rate_;

    timer_ = new ProbingTimer((WormApp *)this);
    timer_->sched(p_inv_);
  }
}

void DnhWormApp::send_probe() {
  int d_addr;
  ns_addr_t dst;

  // do not probe myself
  d_addr = my_addr_;
  while (d_addr == my_addr_)
    d_addr = (int)Random::uniform(total_addr_);

  // probe within my AS
  if (addr_low_ <= d_addr && d_addr <= addr_high_) {
    printf("Node %d is probing node %d, within DN\n", my_addr_, d_addr);
    dst.addr_ = d_addr;
  } else {
    //printf("Node %d is probing node %d, within AN, send to node %d\n", 
    //	   my_addr_, d_addr, default_gw_);
    dst.addr_ = default_gw_;
  }

  dst.port_ = p_port_;
  agent_->sendto((int)p_size_, (const char *)NULL, dst);
}

int DnhWormApp::command(int argc, const char*const* argv) {
  if (argc == 3) {
    if (strcmp(argv[1], "gw") == 0) {
      default_gw_ = atoi(argv[2]);
      return(TCL_OK);
    }
  }
  if (argc == 4) {
    if (strcmp(argv[1], "addr-range") == 0) {
      addr_low_ = atoi(argv[2]);
      addr_high_ = atoi(argv[3]);
      //printf("DN low: %d, high: %d\n", addr_low_, addr_high_);
      return(TCL_OK);
    }
  }

  return(WormApp::command(argc, argv));
}

// class to model vulnerable hosts in detailed network
static class AnWormAppClass : public TclClass {
public:
	AnWormAppClass() : TclClass("Application/Worm/An") {}
	TclObject* create(int, const char*const*) {
		return (new AnWormApp());
	}
} class_app_worm_an;

AnWormApp::AnWormApp() : WormApp() {
  // using 1 second as the unit of time step
  time_step_ = 1;
  timer_ = NULL;

  addr_low_ = addr_high_ = my_addr_;
  s_ = i_ = 0;
  v_percentage_ = 0;
  beta_ = gamma_ = 0;
  n_ = r_ = 1;
  
  probe_in = probe_out = probe_recv = probe_total = 0;

  // get probing port from configuration
  bind("ScanPort", &p_port_);

  // get probing port from configuration
  bind("ScanPacketSize", &p_size_);
}

void AnWormApp::start() {
  // initial value of i and s
  i_ = 1;
  s_ -= 1;

  total_addr_ = (int)pow(2, 32) - 1;

  timer_ = new ProbingTimer((WormApp *)this);
  timer_->sched((double)time_step_);
}

void AnWormApp::recv(int nbytes) {
  probe_recv++;
  
  //printf("AN (%d) received probes from outside...%f \n", my_addr_, probe_recv);
}

void AnWormApp::timeout() {
  timer_->resched((double)time_step_);
  update();
}

void AnWormApp::update() {
  // schedule next timeout
  timer_->resched(time_step_);

  probe_out = beta_ * i_ * (dn_high_ - dn_low_ + 1)  * time_step_ / total_addr_;
  // not every probe received has effect
  probe_in = probe_recv * s_ / n_;
  probe_recv = 0;

  // update states in abstract networks
  // update r (recovered/removed)
  r_ = r_ + gamma_ * i_;
  if (r_ < 0)
    r_ = 0;
  if (r_ > n_)
    r_ = n_;

  // update i (infected)
  // contains four parts:
  // 1. i of last time period, 2. increase due to internal probing
  // 3. decrease due to internal recovery/removal,
  // 4. increase due to external probing

  // should use n_ or s_max_???
  //i_ = i_ + beta_ * i_ * (s_ / n_) * time_step_ - gamma_ * i_ + probe_in;
  i_ = i_ + beta_ * i_ * (s_ / s_max_) * time_step_ - gamma_ * i_ + probe_in;
  if (i_ < 0)
    i_ = 0;
  if (i_ > n_ - r_)
    i_ = n_ - r_;
 
  // update s (susceptible)
  // use n = r + i + s
  s_ = n_ - r_ - i_;

 //printf("ANS %f %f %f %f %f\n", s_, i_, r_, probe_in, probe_out);
	     
  // probe outside networks
  probe_total += probe_out;
  if (probe_total > 1) { 
    printf("ANS %d %d %d %d %d %d\n", 
            (int)Scheduler::instance().clock(), 
	    (int)s_, (int)i_, (int)r_, (int)probe_in, (int)probe_out);
    probe((int)(probe_total + 0.5));
    probe_total = 0;
  }
}

void AnWormApp::probe(int times) {
  // send out probes in a batch
  int i, d_addr;
  ns_addr_t dst;

  i = 0;
  while (i < times) {
    d_addr = dn_low_ + (int)Random::uniform(dn_high_ - dn_low_);

    // do not send to myself or AS
    if (dn_low_ < d_addr && d_addr < dn_high_) {
      // probe outside
      printf("AN is probing node %d, outside AN\n", d_addr);

      dst.addr_ = d_addr;
      dst.port_ = p_port_;
      agent_->sendto((int)p_size_, (const char *)NULL, dst);

      i++;
    }
  }
}

int AnWormApp::command(int argc, const char*const* argv) {
  if (argc == 3) {
    if (strcmp(argv[1], "v_percent") == 0) {
      if (n_ == 0) {
	printf("space range is not specificed!\n");
	return(TCL_ERROR);
      } else {
	v_percentage_ = atof(argv[2]);
	
	s_ = (int)(n_ * v_percentage_);
	if (s_ < 1)
	  s_ = 1;
	s_max_ = s_;
	r_ = n_ - s_;
	
	return(TCL_OK);
      }
    }
    if (strcmp(argv[1], "beta") == 0) {
      beta_ = atof(argv[2]);
      //printf("beta: %f\n", beta_);

      return(TCL_OK);
    }
    if (strcmp(argv[1], "gamma") == 0) {
      gamma_ = atof(argv[2]);
      //printf("gamma: %f\n", gamma_);

      return(TCL_OK);
    }
  }
  if (argc == 4) {
    if (strcmp(argv[1], "addr-range") == 0) {
      addr_low_ = atoi(argv[2]);
      addr_high_ = atoi(argv[3]);

      // initialize SIR model states
      n_ = addr_high_ - addr_low_ + 1;
      //printf("AN low: %d, high: %d, n: %f\n", addr_low_, addr_high_, n_);
      return(TCL_OK);
    }
    if (strcmp(argv[1], "dn-range") == 0) {
      dn_low_ = atoi(argv[2]);
      dn_high_ = atoi(argv[3]);
      //printf("AN-DN low: %d, high: %d\n", dn_low_, dn_high_);

      return(TCL_OK);
    }
  }

  return(WormApp::command(argc, argv));
}
