
/*      proxytrace2txt.c
	
	take ASCII space separated fields and produce binary output file

*/

/*
       Copyright 1996 Digital Equipment Corporation
                        All Rights Reserved

Permission to use, copy, and modify this software and its documentation is
hereby granted only under the following terms and conditions.  Both the
above copyright notice and this permission notice must appear in all copies
of the software, derivative works or modified versions, and any portions
thereof, and both notices must appear in supporting documentation.

Users of this software agree to the terms and conditions set forth herein,
and hereby grant back to Digital a non-exclusive, unrestricted, royalty-
free right and license under any changes, enhancements or extensions 
made to the core functions of the software, including but not limited to 
those affording compatibility with other hardware or software 
environments, but excluding applications which incorporate this software.
Users further agree to use their best efforts to return to Digital any
such changes, enhancements or extensions that they make and inform Digital
of noteworthy uses of this software.  Correspondence should be provided
to Digital at:

                      Director of Licensing
                      Western Research Laboratory
                      Digital Equipment Corporation
                      250 University Avenue
                      Palo Alto, California  94301

This software may not be distributed to third parties.

THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <db.h>		// endian swap macros
#include "proxytrace2txt.h"
 
#ifdef PC
#include <io.h>
#endif


char *ProtocolStr[] =
{
	"NONE",
	"http",
	"ftp",
	"gopher",
	"wais",
	"cache_object",
	"TOTAL"
};

char *RequestMethodStr[] =
{
	"NONE",
	"GET",
	"POST",
	"HEAD",
	"CONNECT"
};


char *   extension_names[] = /* text names for output */
{
	"NONE",
	"html",
	"gif",
	"cgi",  /* 3 */
	"data",
	"class",
	"map",  /* 6 */
	"jpeg",
	"mpeg",
	"OTHER",
	"BAD #"
};
  
/*
 *    main processing routine
 *	goes through entries in event lists
 */

main (int argc, char* argv[]) {

	int            lines_read=0, bytesread, totalread=0, pipe;
	char			*input_file;
	FILE			*in_file;
	char			line [BUFFER_SIZE]; 
 
	TEntry			entry;
	char			 buffer [BUFFER_SIZE];

	/*
	 *	 parse command line
	 *
	 */
	if (argc !=  2)  {
		fprintf(stderr," usage:  proxytrace2txt  inputfile\n");
		exit(-1);
	}
	input_file = argv [1]; 

	/*
	 *		open input file
	 *
	 */
	if (!strncmp (".gz",  input_file+strlen( input_file)-3, 3) )  { 
		sprintf(line,"gzcat %s",  input_file);
		pipe = 1;

#ifdef PC
		in_file = _popen ( line, "r");
#else 
		in_file = popen ( line, "r");
#endif

	}  else {
		pipe = 0;
		in_file = fopen(  input_file,"r");
	}

	if( in_file ==  NULL  ) {
		perror( "Open failed on input file" );
		exit (-1);
	}

	fprintf(stderr, "proxytrace2txt: reading file %s record size %d\n",
		input_file, sizeof (TEntry));
 

	/*
	 *		 go through each line of input changing selected token
	 *
	 */

	fprintf(stdout, "           time   event    server client   ");
	fprintf(stdout, " size last mod status  meth. prot. server  port   type  flags   path  query\n");
 
	bytesread=fread( buffer, sizeof ( char),BUFFER_SIZE,in_file);
	if ( bytesread!= BUFFER_SIZE )  {      /*   */
		fprintf(stderr, " PROXYTRACE2TXT -ERROR unable to read header of size %d\n",
			BUFFER_SIZE);
		perror ("bytes read");
		exit (-1);
	}

	bytesread=fread(&entry,sizeof (entry),1,in_file);
	while  ( bytesread !=  0)   {
		// Convert little endian to big endian
		int i, *p;
		for (p =(int*)&entry,i = 0; i < bytesread; i+=sizeof(int), p++)
			P_32_SWAP(p);

		if (  bytesread <0)  {      /*   */
			perror( "PROXYTRACE2TXT: read failed" );
			exit (-1);
		}
		if ( bytesread!=1 )  {      /*   */
			fprintf(stderr, "proxytrace2txt: ERROR-read fell short %d\n",
				bytesread);
			totalread  += sizeof ( TEntry); 
		}
		if (entry.type >  MAX_EXTENSIONS ) {
			entry.type =  MAX_EXTENSIONS + 1;
		}
		fprintf(stdout, "%9d%06u %8d %8d %5d %7d %9d %6d 56s %6s %6d %6d %6s %6d %6d %6d\n",
			entry.time_sec, entry.time_usec, 
			entry.event_duration, entry.server_duration,
			entry.client, entry.size,  entry.last_mod, 
			entry.status,  RequestMethodStr[entry.method], 
			ProtocolStr[entry.protocol],
			entry.server, entry.port,extension_names [ entry.type],
			entry.flags, 
			entry.path, entry.query);

		totalread  += bytesread; 
		bytesread=fread(&entry,sizeof (entry),1,in_file);
		lines_read++;
	}
	fprintf(stderr, "proxytrace2txt: complete- %d  entries %d bytes\n",  
		lines_read, totalread);
 
	if ( pipe== 0 )  {      /*  do normal close if not pipe  */
		fclose ( in_file); 
	}  else {

#ifdef PC
		_pclose ( in_file);
#else 
		pclose (in_file );
#endif
	}

	return(0);
}
