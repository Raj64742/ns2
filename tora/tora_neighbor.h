/* -*- c++ -*-
   tora_neighbor.h
   $Id: tora_neighbor.h,v 1.1 1999/08/03 04:07:14 yaxu Exp $
  
   */
#ifndef __tora_neighbor_h__
#define __tora_neighbor_h__

enum LinkStatus {
	LINK_UP = 0x0001,	// upstream
	LINK_DN = 0x0002,	// downstream
	LINK_UN = 0x0004,	// undirected
};


class TORANeighbor {
	friend class TORADest;
	friend class toraAgent;
public:
	TORANeighbor(nsaddr_t id, Agent *a); 
	void	update_link_status(Height *h);
	void	dump(void);
private:
	LIST_ENTRY(TORANeighbor) link;

	nsaddr_t	index;

	/*
	 *  height:   The height metric of this neighbor.
	 *  lnk_stat: The assigned status of the link to this neighbor.
	 *  time_act: Time the link to this neighbor became active.
	 */
	Height		height;
	LinkStatus	lnk_stat;
	double		time_act;

        toraAgent     *agent;
};

#endif /* __tora_neighbor_h__ */

