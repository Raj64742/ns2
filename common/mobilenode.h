/* test.h*/
#ifndef __mobilenode_h__
#define __mobilenode_h__

#define POSITION_UPDATE_INTERVAL	30.0	// seconds
#define MAX_SPEED			5.0	// meters per second (33.55 mph)
#define MIN_SPEED			0.0

#include "object.h"
#include "trace.h"
#include "list.h"
#include "phy.h"
#include "topography.h"
#include "arp.h"
#include "node.h"



#if COMMENT_ONLY
		 -----------------------
		|			|
		|	Upper Layers	|
		|			|
		 -----------------------
		    |		    |
		    |		    |
		 -------	 -------
		|	|	|	|
		|  LL	|	|  LL	|
		|	|	|	|
		 -------	 -------
		    |		    |
		    |		    |
		 -------	 -------
		|	|	|	|
		| Queue	|	| Queue	|
		|	|	|	|
		 -------	 -------
		    |		    |
		    |		    |
		 -------	 -------
		|	|	|	|
		|  Mac	|	|  Mac	|
		|	|	|	|
		 -------	 -------
		    |		    |
		    |		    |
		 -------	 -------	 -----------------------
		|	|	|	|	|			|
		| Netif	| <---	| Netif | <---	|	Mobile Node	|
		|	|	|	|	|			|
		 -------	 -------	 -----------------------
		    |		    |
		    |		    |
		 -----------------------
		|			|
		|	Channel(s) 	|
		|			|
		 -----------------------
#endif
		 
class MobileNode;

class PositionHandler : public Handler {
public:
	PositionHandler(MobileNode* n) : node(n) {}
	void handle(Event*);
private:
	MobileNode *node;
};


class MobileNode : public Node 
{
	friend class PositionHandler;
public:
	MobileNode();
	virtual int command(int argc, const char*const* argv);

	double	distance(MobileNode*);
	double	propdelay(MobileNode*);
	void	start(void);
        inline void getLoc(double *x, double *y, double *z) {
		update_position();  *x = X; *y = Y; *z = Z;
	}
        inline void getVelo(double *dx, double *dy, double *dz) {
	  *dx = dX * speed; *dy = dY * speed; *dz = 0.0;
	  // XXX should calculate this for real based on current 
	  // and last Z position
	}
        
	//inline int index() { return index_; }
	inline MobileNode* nextnode() { return link.le_next; }

	void dump(void);
	
	inline MobileNode* base_stn() { return base_stn_; }

protected:
	// Used to generate position updates
	PositionHandler pos_handle;
	Event pos_intr;

	void	log_movement();
	void	update_position();
	void	random_direction();
	void	random_speed();
        void    random_destination();
        int	set_destination(double x, double y, double speed);
	  
	/*
	 * Last time the position of this node was updated.
	 */
	double position_update_time;
        double position_update_interval;

	/*
         *  The following indicate the (x,y,z) position of the node on
         *  the "terrain" of the simulation.
         */
	double X;
	double Y;
	double Z;

	double speed;	// meters per second

private:
	inline int initialized() {
		return (T && log_target &&
			X >= T->lowerX() && X <= T->upperX() &&
			Y >= T->lowerY() && Y <= T->upperY());
	}
	void		random_position();
	void		bound_position();

	//int		index_;		// unique identifier
	int		random_motion_;	// is mobile

	/*
	 * A global list of mobile nodes
	 */
	LIST_ENTRY(MobileNode)	link;

	/*
	 *  A list of network interfaces associated with this
	 *  mobile node.  Each interface is in turn connected
	 *  to a channel  There may be one or more channels.
	 */
	//struct if_head	ifhead_;

	/*
         *  The following is a unit vector that specifies the
         *  direction of the mobile node.  It is used to update
         *  position
         */
	double dX;
	double dY;
	double dZ;

        /* where are we going? */
	double destX;
	double destY;

	/*
	 * The topography over which the mobile node moves.
	 */
	Topography *T;

	/*
	 * Trace Target
	 */
	Trace* log_target;

	/* a base_stn for mobilenodes communicating with 
	 *  wired nodes
	 */
	MobileNode* base_stn_;
};



#endif // mobilenode_h
