#ifndef __tora_dest_h__
#define __tora_dest_h__

class TORANeighbor;
class toraAgent;

LIST_HEAD(tn_head, TORANeighbor);

class TORADest {
	friend class toraAgent;
public:
	TORADest(nsaddr_t id, Agent *a);

	TORANeighbor*	nb_add(nsaddr_t id);
	int		nb_del(nsaddr_t id);
	TORANeighbor*	nb_find(nsaddr_t id);

	void		update_height_nb(TORANeighbor* tn,
					 struct hdr_tora_upd *uh);
	void		update_height(double TAU, nsaddr_t OID,
					int R, int DELTA, nsaddr_t ID);

	int		nb_check_same_ref(void);

	/*
	 *  Finds the minimum height neighbor whose lnk_stat is DN.
	 */
	TORANeighbor*	nb_find_next_hop();

	TORANeighbor*	nb_find_height(int R);
	TORANeighbor*	nb_find_max_height(void);
	TORANeighbor*	nb_find_min_height(int R);
	TORANeighbor*	nb_find_min_nonnull_height(Height *h);

	void		dump(void);

private:
	LIST_ENTRY(TORADest) link;

	nsaddr_t	index;		// IP address of destination
	Height		height;		// 5-tuple height of this node
	int		rt_req;		// route required flag
	double		time_upd;	// time last UPD packet was sent

	double		time_tx_qry;	// time sent last QUERY
	double		time_rt_req;	// time rt_req last set - JGB 

	tn_head		nblist;		// List of neighbors.
	int		num_active;	// # active links
	int		num_down;	// # down stream links
	int		num_up;		// # up stream links

        toraAgent     *agent;
};

#endif // __tora_dest_h__

