#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "logparse.h"

// Read next line into file
int lf_get_next_entry(FILE *fp, lf_entry &ne)
{
	char buf[MAXBUF]; // must be large enough for a cache log

	if ((fgets(buf, MAXBUF, fp) == NULL) || feof(fp) || ferror(fp)) {
		return 1;
	}

	// Parse a line and fill an lf_entry
	char *p = buf, *q, *tmp1, *tmp2;
	u_int32_t lapse;

	// first two entries: <TimeStamp> and <Elapsed Time>
	q = strsep(&p, " ");
	ne.rt = strtod(q, NULL);
	q = strsep(&p, " ");
	lapse = strtoul(q, NULL, 10);
	ne.rt -= (double)lapse/1000.0;

	// Client address
	q = strsep(&p, " ");
	ne.cid = (u_int32_t)inet_addr(q);

	// Log tags, do not store them but use it to filter entries
	tmp1 = q = strsep(&p, " ");
	tmp2 = strsep(&tmp1, "/");
	tmp2 += 4; // Ignore the first 4 char "TCP_"
	if ((strcmp(tmp2, "MISS") == 0) || 
	    (strcmp(tmp2, "CLIENT_REFRESH_MISS") == 0) || 
	    (strcmp(tmp2, "IMS_MISS") == 0) || 
	    (strcmp(tmp2, "DENIED") == 0)) 
		return -1;	// Return negative to discard this entry

	// Page size
	q = strsep(&p, " "); 
	ne.size = strtoul(q, NULL, 10);

	// Request method, GET only
	q = strsep(&p, " ");
	if (strcmp(q, "GET") != 0) 
		return -1;

	// URL
	q = strsep(&p, " ");
	if (strchr(q, '?') != NULL) 
		// Do not accept any URL containing '?'
		return -1;
	ne.url = new char[strlen(q) + 1];
	strcpy(ne.url, q);
	// Try to locate server name from the URL
	tmp1 = strsep(&q, "/");
	if (strcmp(tmp1, "http:") != 0) {
		// How come this isn't a http request???
		delete []ne.url;
		return -1;
	}
	tmp1 = strsep(&q, "/"); 
	tmp1 = strsep(&q, "/");
	ne.sid = new char[strlen(tmp1) + 1];
	strcpy(ne.sid, tmp1);

	// All the rest are useless, do not parse them
	return 0;
}

