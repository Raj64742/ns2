/*
 * Copyright (c) 1991-1994 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and the Network Research Group at
 *	Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Routing code for general topologies based on min-cost routing algorithm in
 * Bertsekas' book.  Written originally by S. Keshav, 7/18/88
 * (his work covered by identical UC Copyright)
 */

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/route.cc,v 1.9 1998/04/17 22:43:39 haldar Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <assert.h>
#include "tclcl.h"
#include "config.h"

#define INFINITY       0x3fff
#define INDEX(i, j, N) ((N) * (i) + (j))

/*** definitions for hierarchical routing support 
right now implemented for 3 levels of hierarchy --> should
be able to extend it for n levels of hierarchy in the future ***/

#define HIER_LEVEL     3
#define N_N_INDEX(i, j, a, b, c)       (((i) * (a+b+c)) + (j))
#define N_C_INDEX(i, j, a, b, c)       (((i) * (a+b+c)) + (a+j))
#define N_D_INDEX(i, j, a, b, c)       (((i) * (a+b+c)) + (a+(b-1)+j))
#define C_C_INDEX(i, j, a, b, c)       (((a+i) * (a+b+c)) + (a+j))
#define C_D_INDEX(i, j, a, b, c)       (((a+i) * (a+b+c)) + (a+(b-1)+j))
#define D_D_INDEX(i, j, a, b, c)       (((a+(b-1)+i) * (a+b+c)) + (a+(b-1)+j))



class RouteLogic : public TclObject {
public:
    RouteLogic();
    ~RouteLogic();
    int command(int argc, const char*const* argv);
protected:
    void check(int);
    void alloc(int n);
    void insert(int src, int dst, int cost);
    void reset(int src, int dst);
    void compute_routes();
    int *adj_;
    int *route_;
    int size_,
	maxnode_;

    /**** Hierarchical routing support ****/
  
    void hier_check(int index);
    void hier_alloc(int size);
    void hier_init(void);
    void str2address(const char*const* address, int *src, int *dst);
    void get_address(char * target, int next_hop, int index, int size, int *src);
    void hier_insert(int *src, int *dst, int cost);
    void hier_reset(int *src, int *dst);
    void hier_compute();
    void hier_compute_routes(int index);

    /* temp testing functions for printing */
    void hier_print_hadj();
    void hier_print_route();
    void hier_print_connect();
    /* for ref : void send-hier-data(int *clus_size, int cluster_num, int domain_num);*/

    int **hadj_;
    int **hroute_;
    int *hsize_;
    int *cluster_size_;        /* no. of nodes/cluster/domain */
    char ***hconnect_;         /* holds the connectivity info --> address of target */
    int level_;
    int C_ ,                   /* no. of clusters/domain */
	D_;                    /* total no. of domains */

};


class RouteLogicClass : public TclClass {
public:
    RouteLogicClass() : TclClass("RouteLogic") {}
    TclObject* create(int, const char*const*) {
	return (new RouteLogic());
    }
} routelogic_class;



int RouteLogic::command(int argc, const char*const* argv)
{
    Tcl& tcl = Tcl::instance();
    if (argc == 2) {
	if (strcmp(argv[1], "compute") == 0) {
	    compute_routes();
	    return (TCL_OK);
	}
	
	if (strcmp(argv[1], "hier-compute") == 0) {
	    hier_compute();
	    return (TCL_OK);
	}
	
	if (strcmp(argv[1], "hier-print") == 0) {
	    hier_print_hadj();
	    return (TCL_OK);
	}
	
	if (strcmp(argv[1], "hier-print-route") == 0) {
	    hier_print_route();
	    return (TCL_OK);
	}
	
	if (strcmp(argv[1], "hier-print-connect") == 0) {
	    hier_print_connect();
	    return (TCL_OK);
	}

	if (strcmp(argv[1], "reset") == 0) {
	    delete[] adj_;
	    adj_ = 0;
	    size_ = 0;
	    return (TCL_OK);
	}
    } 
    else if (argc > 2) {
	
	if (strcmp(argv[1], "insert") == 0) {
	    int src = atoi(argv[2]) + 1;
	    int dst = atoi(argv[3]) + 1;
	    if (src <= 0 || dst <= 0) {
		tcl.result("negative node number");
		return (TCL_ERROR);
	    }
	    int cost = (argc == 5 ? atoi(argv[4]) : 1);
	    insert(src, dst, cost);
	    return (TCL_OK);
	}
	
	if (strcmp(argv[1], "hlevel-is") == 0) {
	    level_ = atoi(argv[2]);
	    if (level_ < 1) {
		tcl.result("send-hlevel: # hierarchy levels should be non-zero");
		return (TCL_ERROR);
	    }
	    return (TCL_OK);
	}
    
	if(strcmp(argv[1], "send-hdata") == 0) {
	    if (argc != (level_ + 1)) {
		tcl.result("send-hdata: # hierarchy levels donot match with # args");
		return (TCL_ERROR);
	    }
	    /***** change this to allow n-levels of hierarchy ***/
	    D_ = atoi(argv[2]) + 1;
	    C_ = atoi(argv[3]) + 1;
	    hier_init();
	    return (TCL_OK);
	}

	if(strcmp(argv[1], "send-hlastdata") == 0) {
	    if (argc != ((C_-1)* (D_-1) + 2)) {
		tcl.result("send-hlastdata: # args do not match");
		return (TCL_ERROR);
	    }
	    int i, j, k = 2;
	    for (i=1; i < D_; i++)
		for (j=1; (j < C_); j++) {
		    cluster_size_[INDEX(i, j, C_)] = atoi(argv[k]);
		    k++;
		}
	    return (TCL_OK);
	}

	if (strcmp(argv[1], "hier-insert") == 0) {
	    if(C_== 0 || D_== 0) {
		tcl.result("Required Hier_data not sent");
		return (TCL_ERROR);
	    }
	    int i,
		n;
	    int  src_addr[SMALL_LEN],
		dst_addr[SMALL_LEN];

	    str2address(argv, src_addr, dst_addr);
	    for (i=0; i < HIER_LEVEL; i++)
		if (src_addr[i]<=0 || dst_addr[i]<=0){
		    tcl.result ("negative node number");
		    return (TCL_ERROR);
		}
	    int cost = (argc == 5 ? atoi(argv[4]) : 1);
	    hier_insert(src_addr, dst_addr, cost);
	    return (TCL_OK);
	}

	if (strcmp(argv[1], "hier-reset") == 0) {
	    int i, n;
	    int  src_addr[SMALL_LEN],
		dst_addr[SMALL_LEN];

	    str2address(argv, src_addr, dst_addr);
	    // assuming node-node addresses (instead of node-cluster or node-domain pair) 
	    // are sent for hier_reset  
	    for (i=0; i < HIER_LEVEL; i++)
		if (src_addr[i]<=0 || dst_addr[i]<=0){
		    tcl.result ("negative node number");
		    return (TCL_ERROR);
		}
	    hier_reset(src_addr, dst_addr);
	}

	if (strcmp(argv[1], "hier-lookup") == 0) {
	    int i;
	    int  src[SMALL_LEN],
		dst[SMALL_LEN];
      
	    if ( hroute_ == 0) {
		tcl.result("Required Hier_data not sent");
		return (TCL_ERROR);
	    }
      
	    str2address(argv, src, dst);
	    for (i=0; i < HIER_LEVEL; i++)
		if (src[i] <= 0) {
		    tcl.result ("negative src node number");
		    return (TCL_ERROR);
		}
	    if (dst[0] <= 0) {
		tcl.result ("negative dst domain number");
		return (TCL_ERROR);
	    }

	    int index = INDEX(src[0], src[1], C_);
	    int size = cluster_size_[index];

	    if (hsize_[index] == 0) {
		tcl.result ("Routes not computed");
		return (TCL_ERROR);
	    }
	    if ((src[0] < D_) || (dst[0] < D_)) {
		if((src[1] < C_) || (dst[1] < C_))
		    if((src[2] <= size) ||
		       (dst[2] <= cluster_size_[INDEX(dst[0], dst[1], C_)]))
			;
		 	// printf("node within range\n");
	    }
	    else { 
		tcl.result("node out of range");
		return (TCL_ERROR);
	    }
	    int next_hop = 0;
	    /* if node-domain lookup */
	    if ((dst[1] <= 0) && (dst[2] <= 0)) {
		next_hop = hroute_[index][N_D_INDEX(src[2], dst[0], size, C_, D_)];
		// printf("Node-domain lookup:Next hop = %d\n",next_hop);
	    }
      
	    /* if node-cluster lookup */
	    else if (dst[2] <= 0) {
		next_hop = hroute_[index][N_C_INDEX(src[2], dst[1], size, C_, D_)];
// 		printf("Node-cluster lookup:Next hop = %d\n",next_hop);
	    }
      
	    /* if node-node lookup */
	    else {
		next_hop = hroute_[index][N_N_INDEX(src[2], dst[2], size, C_, D_)];	
// 		printf("Node-node lookup:Next hop = %d\n",next_hop);
	    }
	    char target[SMALL_LEN];
	    get_address(target, next_hop, index, size, src);
// 	    printf("Target = %s\n",target);
	    tcl.resultf("%s",target);	   
	    return (TCL_OK);
	}
    

	if (strcmp(argv[1], "reset") == 0) {
	    int src = atoi(argv[2]) + 1;
	    int dst = atoi(argv[3]) + 1;
	    if (src <= 0 || dst <= 0) {
		tcl.result("negative node number");
		return (TCL_ERROR);
	    }
	    reset(src, dst);
	    return (TCL_OK);
	}
	if (strcmp(argv[1], "lookup") == 0) {
	    if (route_ == 0) {
		tcl.result("routes not computed");
		return (TCL_ERROR);
	    }
	    int src = atoi(argv[2]) + 1;
	    int dst = atoi(argv[3]) + 1;
	    if (src >= size_ || dst >= size_) {
		tcl.result("node out of range");
		return (TCL_ERROR);
	    }
	    tcl.resultf("%d", route_[INDEX(src, dst, size_)] - 1);
	    return (TCL_OK);
	}
    }
    return (TclObject::command(argc, argv));
}
    
RouteLogic::RouteLogic()
{
    size_ = 0;
    adj_ = 0;
    route_ = 0;
    /* additions for hierarchical routing extension */
    C_ = 0;
    D_ = 0;
    level_ = 0;
    hsize_ = 0;
    hadj_ = 0;
    hroute_ = 0;
    hconnect_ = 0;
    cluster_size_ = 0;
	
}
	
RouteLogic::~RouteLogic()
{
    delete[] adj_;
    delete[] route_;
    for (int x=1; x < D_; x++) {
	for (int y=1; y < C_; y++) {
	    int i = INDEX(x, y, C_);
	    int n = cluster_size_[i];
	    for (int j=0; j < (n + 1) * (C_+ D_); j++) {
		if (hconnect_[i][j] != NULL)
		    delete [] hconnect_[i][j];
	    }
	}
    }
    for (int n =0; n < (C_ * D_); n++) {
	if (hadj_[n] != NULL)
	    delete hadj_[n];
	if (hroute_[n] != NULL)
	    delete hroute_[n];
	if (hconnect_[n] != NULL)
	    delete hconnect_[n];
    }
    delete [] hsize_;
    delete [] cluster_size_;
    delete [] hadj_;
    delete [] hroute_;
    delete [] hconnect_;
}

void RouteLogic::alloc(int n)
{
    size_ = n;
    n *= n;
    adj_ = new int[n];
    for (int i = 0; i < n; ++i)
	adj_[i] = INFINITY;
}

/*
 * Check that we have enough storage in the adjacency array
 * to hold a node numbered "n"
 */
void RouteLogic::check(int n)
{
    if (n < size_)
	return;
    
    int* old = adj_;
    int osize = size_;
    int m = osize;
    if (m == 0)
	m = 16;
    while (m <= n)
	m <<= 1;

    alloc(m);
    for (int i = 0; i < osize; ++i) {
	for (int j = 0; j < osize; ++j)
	    adj_[INDEX(i, j, m)] = old[INDEX(i, j, osize)];
    }
    size_ = m;
    delete[] old;
}

void RouteLogic::insert(int src, int dst, int cost)
{
    check(src);
    check(dst);
    adj_[INDEX(src, dst, size_)] = cost;
}

void RouteLogic::reset(int src, int dst)
{
    assert(src < size_);
    assert(dst < size_);
    adj_[INDEX(src, dst, size_)] = INFINITY;
}

void RouteLogic::compute_routes()
{
    int n = size_;
    int* hopcnt = new int[n];
    int* parent = new int[n];
#define ADJ(i, j) adj_[INDEX(i, j, size_)]
#define ROUTE(i, j) route_[INDEX(i, j, size_)]
    delete[] route_;
    route_ = new int[n * n];
    memset((char *)route_, 0, n * n * sizeof(route_[0]));
    
    /* do for all the sources */
    int k;
    for (k = 1; k < n; ++k) {
	int v;
	for (v = 0; v < n; v++)
	    parent[v] = v;
	
	/* set the route for all neighbours first */
	for (v = 1; v < n; ++v) {
	    if (parent[v] != k) {
		hopcnt[v] = ADJ(k, v);
		if (hopcnt[v] != INFINITY)
		    ROUTE(k, v) = v;
	    }
	}
	for (v = 1; v < n; ++v) {
	    /*
	     * w is the node that is the nearest to the subtree
	     * that has been routed
	     */
	    int o = 0;
	    /* XXX */
	    hopcnt[0] = INFINITY;
	    int w;
	    for (w = 1; w < n; w++)
		if (parent[w] != k && hopcnt[w] < hopcnt[o])
		    o = w;
	    parent[o] = k;
	    /*
	     * update distance counts for the nodes that are
	     * adjacent to o
	     */
	    if (o == 0)
		continue;
	    for (w = 1; w < n; w++) {
		if (parent[w] != k &&
		    hopcnt[o] + ADJ(o, w) < hopcnt[w]) {
		    ROUTE(k, w) = ROUTE(k, o);
		    hopcnt[w] = hopcnt[o] + ADJ(o, w);
		}
	    }
	}
    }
    /*
     * The route to yourself is yourself.
     */
    for (k = 1; k < n; ++k)
	ROUTE(k, k) = k;
    
    delete[] hopcnt;
    delete[] parent;
}



/* hierarchical routing support */
/* This function creates adjacency matrix for each cluster at the lowest level of the hierarchy for every node in the cluster, for every other cluster in the domain, and every other domain. can be extended from 3-level hierarchy to n-level along similar lines*/


void RouteLogic::hier_alloc(int i)
{
    // printf("Comes to hier_alloc..\n");
  
    hsize_[i] = cluster_size_[i]+ C_+ D_ ;
    // hsize_[i] = hsize_[i] * (cluster_size_[i]+1); 
    hsize_[i] *= hsize_[i];
    hadj_[i] = new int[hsize_[i]];
    hroute_[i] = new int[hsize_[i]];
    // hconnect_[i] = new (char*)[hsize_[i]];
    hconnect_[i] = new (char*)[(C_+D_) * (cluster_size_[i]+1)];
    for (int n = 0; n < hsize_[i]; n++){
	hadj_[i][n] = INFINITY;
	hroute_[i][n] = INFINITY;
    }
//     printf("Passes hier_alloc..\n");
}


void RouteLogic::hier_check(int i)
{
    // printf("Hier_size[%d] = %d\n",i,hsize_[i]);
    if(hsize_[i] > 0)
	return;
    else
	hier_alloc(i);
}

void RouteLogic::hier_init(void) 
{
    int arr_size = C_* D_ ;
    cluster_size_ = new int[arr_size]; 
    hsize_ = new int[arr_size];
    for (int i = 0; i < arr_size; i++)
	hsize_[i] = 0;
    hadj_ = new (int *)[arr_size];
    hroute_ = new (int *)[arr_size];
    hconnect_ = new (char **)[arr_size];
}   

void RouteLogic::str2address(const char*const* argv, int *src_addr, int *dst_addr)
{
    int  i, n;
    char tmpstr[SMALL_LEN];
    char *next,
	*index;
    char *addr[2];

    /* initializing src and dst addr */
    for (i=0; i < SMALL_LEN; i++){
	src_addr[i] = 0;
	dst_addr[i] = 0;
    }
  
    for (i=0, n=2; n<=3; i++,n++){
	addr[i] = new char;
	strcpy(addr[i], argv[n]);
//	printf("addr[%d] = %s\n",i,addr[i]);
    }

    for (n=0; n < 2; n++) {
	i=0;
	strcpy(tmpstr, addr[n]);
	next = tmpstr;
	while(next){
	    index = strstr(next, ".");
	    if (index != NULL){
		next[index - next] = '\0';
		if (n == 0)
		    src_addr[i] = atoi(next) + 1;
		else
		    dst_addr[i] = atoi(next) + 1;
		next = index + 1;
		i++;
	    }
	    else {
		if(n == 0)
		    src_addr[i] = atoi(next) + 1;
		else
		    dst_addr[i] = atoi(next) + 1;
		break;
	    }
	}
    }
    // TESTING
//     for (n=0; n < 3; n++) {
// 	printf("src_addr[%d] = %d\n",n,src_addr[n]);
// 	printf("dst_addr[%d] = %d\n",n,dst_addr[n]);
//     }
    
    delete [] addr;
}


void RouteLogic::get_address(char *address, int next_hop, int index, int size, int *src)
{
//     printf("size = %d\n", size);
    if (next_hop <= size) {
	sprintf(address,"%d.%d.%d", src[0]-1, src[1]-1, next_hop-1);
// 	printf("target address = %s\n",address);
    }
    else if ((next_hop > size) && (next_hop < (size+C_))) {
	int temp = next_hop-size;
	int I = src[2] * (C_+D_) + temp;
	strcpy(address, hconnect_[index][I]);
// 	printf("target address = %s\n", address);
    }
    else {
	int temp = next_hop-size-(C_-1);
	int I = src[2] * (C_+D_) + (C_- 1 +temp);
	strcpy(address,hconnect_[index][I]);
// 	printf("target address = %s\n", address);
    }
}


void RouteLogic::hier_reset(int *src, int *dst)
{
    int i, n;
    if (src[0] == dst[0])
	if (src[1] == dst[1]) {
	    i = INDEX(src[0], src[1], C_);
	    n = cluster_size_[i];
	    hadj_[i][N_N_INDEX(src[2], dst[2], n, C_, D_)] = INFINITY;
	}
  
	else {
	    for (int y=1; y < C_; y++) { 
		i = INDEX(src[0], y, C_);
		n = cluster_size_[i];
		hadj_[i][C_C_INDEX(src[1], dst[1], n, C_, D_)] = INFINITY;
		if (y == src[1])
		    hadj_[i][N_C_INDEX(src[2], dst[1], n, C_, D_)] = INFINITY; 
	    }
	}
    else {
	for (int x=1; x < D_; x++)  
	    for (int y=1; y < C_; y++) {
		i = INDEX(x, y, C_);
		n = cluster_size_[i];
		hadj_[i][D_D_INDEX(src[0], dst[0], n, C_, D_)] = INFINITY;
		if ( x == src[0] ){
		    hadj_[i][C_D_INDEX(src[1], dst[0], n, C_, D_)] = INFINITY;
		    if (y == src[1])
			hadj_[i][N_D_INDEX(src[2], dst[0], n, C_, D_)] = INFINITY;
		}
	    }
    }
}

void RouteLogic::hier_insert(int *src_addr, int *dst_addr, int cost)
{
  
    int X1 = src_addr[0];
    int Y1 = src_addr[1];
    int Z1 = src_addr[2];
    int X2 = dst_addr[0];
    int Y2 = dst_addr[1];
    int Z2 = dst_addr[2];
    int n,
	i;

    // printf("src_addr:%d.%d.%d\ndst_addr:%d.%d.%d\n\n",X1,Y1,Z1,X2,Y2,Z2);
    if ( X1 == X2)
	if (Y1 == Y2){ 
	    /*
	     * For the same domain & cluster 
	     */
	    // printf("Same domain & same cluster..\n");
	    i = INDEX(X1, Y1, C_);
	    n = cluster_size_[i];
	    hier_check(i);
	    hadj_[i][N_N_INDEX(Z1, Z2, n, C_, D_)] = cost;
	    // printf("%d - %d => pos(%d)\n",Z1, Z2, C_C_INDEX(Z1, Z2, n, C_, D_));
	}
  
	else { 
	    /* 
	     * For the same domain but diff clusters 
	     */
	    // printf("Same domain but diff. cluster..\n"); 
	    for (int y=1; y < C_; y++) { /* insert cluster connectivity */
		i = INDEX(X1, y, C_);
		n = cluster_size_[i];
		hier_check(i);
		hadj_[i][C_C_INDEX(Y1, Y2, n, C_, D_)] = cost;
		// printf("%d - %d => pos(%d)\n",Y1,Y2,C_C_INDEX(Y1, Y2, n, C_, D_));

		if (y == Y1) {  /* insert node conn. */
		    hadj_[i][N_C_INDEX(Z1, Y2, n, C_, D_)] = cost;
		    int I = Z1 * (C_+ D_) + Y2;
		    hconnect_[i][I] = new char[SMALL_LEN];
		    sprintf(hconnect_[i][I],"%d.%d.%d",X2-1,Y2-1,Z2-1);
		    // printf("Added hconnect: at [%d][%d] = %s\n",i, I, hconnect_[i][I]);
		}
	    }
	}
    
    else { 
	/* 
	 * For different domains 
	 */
	// printf("Diff. domain..\n"); 
	for (int x=1; x < D_; x++) { /* inset domain connectivity */
	    for (int y=1; y < C_; y++) {
		i = INDEX(x, y, C_);
		n = cluster_size_[i];
		/* printf("index = %d\ncluster_size = %d\n",i,n); */
		hier_check(i);
		hadj_[i][D_D_INDEX(X1, X2, n, C_, D_)] = cost;
		// printf("%d - %d => pos(%d)\n",X1,X2,D_D_INDEX(X1, X2, n, C_, D_));
	    }
	}
	for (int y=1; y < C_; y++) { /* insert cluster connectivity */
	    i = INDEX(X1, y, C_);
	    n = cluster_size_[i];
	    hier_check(i);
	    hadj_[i][C_D_INDEX(Y1, X2, n, C_, D_)] = cost;
	    // printf("%d - %d => pos(%d)\n",Y1,X2,C_D_INDEX(Y1, X2, n, C_, D_));
	}
	/* insert node connectivity */
	i = INDEX(X1, Y1, C_);
	n = cluster_size_[i]; 
	hadj_[i][N_D_INDEX(Z1, X2, n, C_, D_)] = cost;
	int I = Z1 * (C_+ D_) + (C_- 1 + X2);
	hconnect_[i][I] = new char[SMALL_LEN];
	sprintf(hconnect_[i][I],"%d.%d.%d",X2-1,Y2-1,Z2-1);
	// printf("Added hconnect: at [%d][%d] = %s\n",i, I, hconnect_[i][I]);
    }
  
}


void RouteLogic::hier_compute_routes(int i)
{
    int size = (cluster_size_[i]+C_+D_);
    int n = size ;
    int* hopcnt = new int[n];
    int* parent = new int[n];
#define HADJ(i, j) adj_[INDEX(i, j, size)]
#define HROUTE(i, j) route_[INDEX(i, j, size)]
    delete[] route_;
    route_ = new int[n * n];
    memset((char *)route_, 0, n * n * sizeof(route_[0]));

    /* do for all the sources */
    int k;
    for (k = 1; k < n; ++k) {
	int v;
	for (v = 0; v < n; v++)
	    parent[v] = v;

	/* set the route for all neighbours first */
	for (v = 1; v < n; ++v) {
	    if (parent[v] != k) {
		hopcnt[v] = HADJ(k, v);
		if (hopcnt[v] != INFINITY)
		    HROUTE(k, v) = v;
	    }
	}
	for (v = 1; v < n; ++v) {
	    /*
	     * w is the node that is the nearest to the subtree
	     * that has been routed
	     */
	    int o = 0;
	    /* XXX */
	    hopcnt[0] = INFINITY;
	    int w;
	    for (w = 1; w < n; w++)
		if (parent[w] != k && hopcnt[w] < hopcnt[o])
		    o = w;
	    parent[o] = k;
	    /*
	     * update distance counts for the nodes that are
	     * adjacent to o
	     */
	    if (o == 0)
		continue;
	    for (w = 1; w < n; w++) {
		if (parent[w] != k &&
		    hopcnt[o] + HADJ(o, w) < hopcnt[w]) {
		    HROUTE(k, w) = HROUTE(k, o);
		    hopcnt[w] = hopcnt[o] + HADJ(o, w);
		}
	    }
	}
    }
    /*
     * The route to yourself is yourself.
     */
    for (k = 1; k < n; ++k)
	HROUTE(k, k) = k;

    delete[] hopcnt;
    delete[] parent;
}



/* function to check the adjacency matrices created */
void RouteLogic::hier_print_hadj() {
    int i, j, k;

    for (j=1; j < D_; j++) 
	for (k=1; k < C_; k++) {
	    i = INDEX(j, k, C_);
	    int s = (cluster_size_[i] + C_+ D_);
	    printf("ADJ MATRIX[%d] for cluster %d.%d :\n",i,j,k);
	    int temp = 1;
	    printf(" ");
	    while(temp < s){
		printf(" %d",temp);
		temp++;
	    }
	    printf("\n");
	    for(int n=1; n < s;n++){
		printf("%d ",n);
		for(int m=1;m < s;m++){
		    if(hadj_[i][INDEX(n,m,s)] == INFINITY)
			printf("~ ");
		    else
			printf("%d ",hadj_[i][INDEX(n,m,s)]);
		}
		printf("\n");
	    }
	    printf("\n\n");
	}
}


void RouteLogic::hier_compute()
{
    int i, j, k;
    for (j=1; j < D_; j++) 
	for (k=1; k < C_; k++) {
	    i = INDEX(j, k, C_);
	    int s = (cluster_size_[i]+C_+D_);
	    adj_ = new int[(s * s)];
	    memset((char *)adj_, 0, s * s * sizeof(adj_[0]));
	    for (int n=0; n < s; n++)
		for(int m=0; m < s; m++)
		    adj_[INDEX(n, m, s)] = hadj_[i][INDEX(n, m, s)];
	    hier_compute_routes(i);
	
	    for (int n=0; n < s; n++)
		for(int m=0; m < s; m++)
		    hroute_[i][INDEX(n, m, s)] = route_[INDEX(n, m, s)];
	    delete [] adj_;
	}
}


void RouteLogic::hier_print_route()
{
    for (int j=1; j < D_; j++) 
	for (int k=1; k < C_; k++) {
	    int i = INDEX(j, k, C_);
	    int s = (cluster_size_[i]+C_+D_);
	    printf("ROUTE_TABLE[%d] for cluster %d.%d :\n",i,j,k);
	    int temp = 1;
	    printf(" ");
	    while(temp < s){
		printf(" %d",temp);
		temp++;
	    }
	    printf("\n");
	    for(int n=1; n < s; n++){
		printf("%d ",n);
		for(int m=1; m < s; m++)
		    printf("%d ",hroute_[i][INDEX(n, m, s)]);
		printf("\n");
	    }
	    printf("\n\n");
	}
}

void RouteLogic::hier_print_connect()
{
    
    for (int j=1; j < D_; j++) 
	for (int k=1; k < C_; k++) {
	    int i = INDEX(j, k, C_);
	    int s = (cluster_size_[i]);
	    printf("size = %d\n",s);
	    printf("CONNECT_TABLE[%d] for cluster %d.%d :\n",i,j,k);
	    printf(" ");
	    int temp = 1;
	    while(temp < C_) {
		printf("     C%d",temp);
		temp++;
	    }
	    temp =1;
	    while(temp < D_) {
		printf("     D%d",temp);
		temp++;
	    }
	    int t = C_+D_;
	    printf("\n");
	    for(int n=1; n < s+1; n++){
		printf("%d ",n);
		for(int m=1; m < t; m++) {
		    // if (hconnect_[i][INDEX(n, m, t)] != NULL) {
		    printf(" %d ",INDEX(n, m, t));
		    printf("%s ",hconnect_[i][INDEX(n, m, t)]);
		}
		printf("\n");
	    }
	    printf("\n\n");
	}
}
	    
