
/* Ported from CMU/Monarch's code, nov'98 -Padma.
   topography.h

*/

#ifndef ns_topography_h
#define ns_topography_h

#include <object.h>

class Topography : public TclObject {

public:
	Topography() { maxX = maxY = grid_resolution = 0.0; grid = 0; }

	double	lowerX() { return 0.0; }
	double	upperX() { return maxX * grid_resolution; }
	double	lowerY() { return 0.0; }
	double	upperY() { return maxY * grid_resolution; }
	double	resol() { return grid_resolution; }
	double	height(double x, double y);
private:
	virtual int command(int argc, const char*const* argv);
	int	load_flatgrid(int x, int y, int res = 1);
	int	load_demfile(const char *fname);

	double	maxX;
	double	maxY;

	double	grid_resolution;
	int*	grid;
};

#endif // ns_topography_h
