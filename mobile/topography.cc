/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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
 */
/* Ported from CMU/Monarch's code, nov'98 -Padma.*/

#include <math.h>
#include <stdlib.h>

#include "object.h"

#include "dem.h"
#include "topography.h"

/* -NEW- */
#include <god.h>
#include "wireless-phy.h"
#include <string.h>
#include <mobilenode.h>
// For TwoRayGetDist(...) - function
#include "tworayground.h"
/* End -NEW- */

static class TopographyClass : public TclClass {
public:
        TopographyClass() : TclClass("Topography") {}
        TclObject* create(int, const char*const*) {
                return (new Topography);
        }
} class_topography;


double
Topography::height(double x, double y) {

	if(grid == 0)
		return 0.0;

#ifndef WIN32
	int a = (int) rint(x/grid_resolution);
	int b = (int) rint(y/grid_resolution);
	int c = (int) rint(maxY);
#else 
	int a = (int) ceil(x/grid_resolution+0.5);
	int b = (int) ceil(y/grid_resolution+0.5);
	int c = (int) ceil(maxY+0.5);
#endif /* !WIN32 */

	return (double) grid[a * c + b];
}


int
Topography::load_flatgrid(int x, int y, int res)
{
	/* No Reason to malloc a grid */

	grid_resolution = res;	// default is 1 meter
	maxX = (double) x;
	maxY = (double) y;

	return 0;
}


int
Topography::load_demfile(const char *fname)
{
	DEMFile *dem;

	fprintf(stderr, "Opening DEM file %s...\n", fname);

	dem = new DEMFile((char*) fname); 
	if(dem == 0)
		return 1;

	fprintf(stderr, "Processing DEM file...\n");

	grid = dem->process();
	if(grid == 0) {
		delete dem;
		return 1;
	}
	dem->dump_ARecord();

	double tx, ty; tx = maxX; ty = maxY;
	dem->range(tx, ty);			// Get the grid size
	maxX = tx; maxY = ty;

	dem->resolution(grid_resolution);	// Get the resolution of each entry

	/* Close the DEM file */
	delete dem;

	/*
	 * Sanity Check
	 */
	if(maxX <= 0.0 || maxY <= 0.0 || grid_resolution <= 0.0)
		return 1;

	fprintf(stderr, "DEM File processing complete...\n");

	return 0;
}

/* -NEW- */
MobileNode **
Topography::getAffectedNodes(MobileNode *mn, double radius,
                             int *numAffectedNodes)
{
	
	double xmin, xmax, ymin, ymax;
	int n = 0;
	MobileNode *tmp, **list, **tmpList;
 
        if(MobileNode::xListHead == NULL){ // yListHead is also null then
		*numAffectedNodes=-1;
		//fprintf(stderr, "xListHead is NULL when trying to send!!!\n");
		return NULL;
	}

	 xmin = mn->X() - radius;
	 xmax = mn->X() + radius;
	 ymin = mn->Y() - radius;
	 ymax = mn->Y() + radius;
 
 
         // First allocate as much as possibly needed
         tmpList = new MobileNode*[numNodes];


	 for(tmp = mn; tmp != NULL && tmp->X() >= xmin; tmp=tmp->prevX)
		 if(tmp->Y() >= ymin && tmp->Y() <= ymax){
			 tmpList[n++] = tmp;
		 }
	 for(tmp = mn->nextX; tmp != NULL && tmp->X() <= xmax; tmp=tmp->nextX){
		 if(tmp->Y() >= ymin && tmp->Y() <= ymax){
			 tmpList[n++] = tmp;
		 }
	 }


         list = new MobileNode*[n];
         memcpy(list, tmpList, n * sizeof(MobileNode *));
         delete [] tmpList;
         
	 *numAffectedNodes = n;
	 return list;
}
 

bool Topography::sorted = false;
void 
Topography::sortLists(void){
  bool flag = true;
  MobileNode *m, *q;
  numNodes = God::instance()->nodes();
 
  sorted = true;

  fprintf(stderr, "SORTING LISTS ...");
  /* Buble sort algorithm */
  // SORT x-list
  while(flag) {
    flag = false;
    m = MobileNode::xListHead;
    while (m != NULL){
	    if(m->nextX != NULL)
		    if ( m->X() > m->nextX->X() ){
			    flag = true;
			    //delete_after m;
			    q = m->nextX;
			    m->nextX = q->nextX;
			    if (q->nextX != NULL)
				    q->nextX->prevX = m;
			    
			    //insert_before m;
			    q->nextX = m;
			    q->prevX = m->prevX;
			    m->prevX = q;
			    if (q->prevX != NULL)
				    q->prevX->nextX = q;

			    // adjust Head of List
			    if(m == MobileNode::xListHead) MobileNode::xListHead = m->prevX;
		    }
	    m = m -> nextX;
    }
  }
  
  fprintf(stderr, "DONE!\n");
}

void 
Topography::updateNodesLists(class MobileNode* mn, double oldX)
{
	MobileNode* tmp;
	double X = mn->X();
	//double Y = mn->Y();
	bool skipX=false;
	//bool skipY=false;
	//int i;

	if(!sorted) {
		sortLists();
		return;
	}

	/* xListHead and yListHead cannot be NULLs here (they are created in MobileNode constructor) */
	
	/***  DELETE ***/
	// deleting mn from x-list
	if(mn->nextX != NULL) {
		if(mn->prevX != NULL){
			if((mn->nextX->X() >= X) && (mn->prevX->X() <= X)) skipX = true; // the node doesn't change its position in the list
			else{
				mn->nextX->prevX = mn->prevX;
				mn->prevX->nextX = mn->nextX;
			}
		}
		else{
			if(mn->nextX->X() >= X) skipX = true; // skip updating the first element
			else{
				mn->nextX->prevX = NULL;
				MobileNode::xListHead = mn->nextX;
			}
		}
	}
	else if(mn->prevX !=NULL){
		if(mn->prevX->X() <= X) skipX = true; // skip updating the last element
		else mn->prevX->nextX = NULL;
	}



	/*** INSERT ***/
	//inserting mn in x-list
	if(!skipX){
		if(X > oldX){			
			for(tmp = mn; tmp->nextX != NULL && tmp->nextX->X() < X; tmp = tmp->nextX);
			//fprintf(stdout,"Scanning the element addr %d X=%0.f, next addr %d X=%0.f\n", tmp, tmp->X(), tmp->nextX, tmp->nextX->X());
			if(tmp->nextX == NULL) { 
				//fprintf(stdout, "tmp->nextX is NULL\n");
				tmp->nextX = mn;
				mn->prevX = tmp;
				mn->nextX = NULL;
			} 
			else{ 
				//fprintf(stdout, "tmp->nextX is not NULL, tmp->nextX->X()=%0.f\n", tmp->nextX->X());
				mn->prevX = tmp->nextX->prevX;
				mn->nextX = tmp->nextX;
				tmp->nextX->prevX = mn;  	
				tmp->nextX = mn;
			} 
		}
		else{
			for(tmp = mn; tmp->prevX != NULL && tmp->prevX->X() > X; tmp = tmp->prevX);
				//fprintf(stdout,"Scanning the element addr %d X=%0.f, prev addr %d X=%0.f\n", tmp, tmp->X(), tmp->prevX, tmp->prevX->X());
			if(tmp->prevX == NULL) {
				//fprintf(stdout, "tmp->prevX is NULL\n");
				tmp->prevX = mn;
				mn->nextX = tmp;
				mn->prevX = NULL;
				MobileNode::xListHead = mn;
			} 
			else{
				//fprintf(stdout, "tmp->prevX is not NULL, tmp->prevX->X()=%0.f\n", tmp->prevX->X());
				mn->nextX = tmp->prevX->nextX;
				mn->prevX = tmp->prevX;
				tmp->prevX->nextX = mn;  	
				tmp->prevX = mn;		
			}
		}
	}
}
	

/* END -NEW- */



int
Topography::command(int argc, const char*const* argv)
{
	if(argc == 3) {

		if(strcmp(argv[1], "load_demfile") == 0) {
			if(load_demfile(argv[2]))
				return TCL_ERROR;
			return TCL_OK;
		}
	}
	else if(argc == 4) {
		if(strcmp(argv[1], "load_flatgrid") == 0) {
			if(load_flatgrid(atoi(argv[2]), atoi(argv[3])))
				return TCL_ERROR;
			return TCL_OK;
		}
	}
	else if(argc == 5) {
		if(strcmp(argv[1], "load_flatgrid") == 0) {
			if(load_flatgrid(atoi(argv[2]), atoi(argv[3]), atoi(argv[4])))
				return TCL_ERROR;
			return TCL_OK;
		}
	}
	return TclObject::command(argc, argv);
}
