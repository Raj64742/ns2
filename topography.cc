#include <math.h>
#include <stdlib.h>

#include <object.h>

#include <dem.h>
#include <topography.h>

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

	int a = (int) rint(x/grid_resolution);
	int b = (int) rint(y/grid_resolution);
	int c = (int) rint(maxY);

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

int
Topography::command(int argc, const char*const* argv)
{
	if(argc == 3) {

		if(strncasecmp(argv[1], "load_demfile", 12) == 0) {
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
