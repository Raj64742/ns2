/* 
   propagation.cc
   $Id: propagation.cc,v 1.1 1998/12/08 00:17:14 haldar Exp $
   */

#include <stdio.h>

#include <topography.h>
#include <propagation.h>
#include <wireless-phy.h>

class PacketStamp;
int
Propagation::command(int argc, const char*const* argv)
{
  TclObject *obj;  

  if(argc == 3) 
    {
      if( (obj = TclObject::lookup(argv[2])) == 0) 
	{
	  fprintf(stderr, "Propagation: %s lookup of %s failed\n", argv[1],
		  argv[2]);
	  return TCL_ERROR;
	}

      if (strcasecmp(argv[1], "topography") == 0) 
	{
	  topo = (Topography*) obj;
	  return TCL_OK;
	}
    }
  return TclObject::command(argc,argv);
}
 

/* As new network-intefaces are added, add a default method here */

double
Propagation::Pr(PacketStamp *, PacketStamp *, Phy *)
{
	fprintf(stderr,"Propagation model %s not implemented for generic NetIF\n",
	  name);
  abort();
}


double
Propagation::Pr(PacketStamp *, PacketStamp *, WirelessPhy *)
{
  fprintf(stderr,
	  "Propagation model %s not implemented for SharedMedia interface\n",
	  name);
  abort();
}
