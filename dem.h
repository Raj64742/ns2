#ifndef __dem_h__
#define __dem_h__

struct ARecord {
	char	q_name[145];
	int	dl_code;
	int	p_code;
	int	pr_code;
	int	z_code;
	float	p_parm[15];
	int	g_units;	/* ground planimetric coordinates */
	int	e_units;	/* elevation coordinates */
	int	sides;
	float	corners[4][2];

	float	min_elevation;	/* minimum and maximum elevations */
	float	max_elevation;

	float	angle;
	int	a_code;		/* Accuracy code */

	float	x_res;		/* Spatial Resolution */
	float	y_res;
	float	z_res;

	int	rows;		/* number of rows and columns of profiles */
	int	cols;
};

struct BRecord {
	int	row_id;
	int	col_id;
	int	rows;
	int	cols;
	float	x_gpc;
	float	y_gpc;
	float	elevation;
	float	min_elevation;
	float	max_elevation;
};


class DEMFile {
public:
	DEMFile(char *s) : fname(s), demfile(0) { }
	~DEMFile() { if(demfile) fclose(demfile); }
	int*	process(void);
	void	dump_ARecord(void);
	void	dump_BRecord(void);
	void	dump_grid(void);
	void	range(double &x, double &y);
	void	resolution(double &r);

private:
	int	open(void);
	int	read_int(void);
	float	read_float(void);
	void	read_field(void);

	char		*fname;
	FILE		*demfile;
	struct ARecord	a;
	struct BRecord	b;
	int		*grid;
	char		tempbuf[1024];
};

#endif /* __dem_h__ */

