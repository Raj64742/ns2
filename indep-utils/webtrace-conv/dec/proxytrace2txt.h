/*
 *	  proxytraces2txt.h
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

#ifndef _PROXYTRACES2TXT_
#define _PROXYTRACES2TXT_

#define  BUFFER_SIZE 8192
#define  MAXLINE  4096

/* unsigned number - 4 bytes */
typedef  unsigned int u_4bytes;


typedef enum {
    METHOD_NONE= 0,
    METHOD_GET,
    METHOD_POST,
    METHOD_HEAD,
    METHOD_CONNECT
} method_t;

extern char *RequestMethodStr[];

typedef enum {
    PROTO_NONE = 0,
    PROTO_HTTP,
    PROTO_FTP,
    PROTO_GOPHER,
    PROTO_WAIS,
    PROTO_CACHEOBJ,
    PROTO_MAX
} protocol_t;

extern char *ProtocolStr[];

#define  NO_PATH_FLAG 1    /* path consists of only / */
#define  PORT_SPECIFIED_FLAG 2
#define  NULL_PATH_ADDED_FLAG 4
#define  QUERY_FOUND_FLAG 8
#define  EXTENSION_SPECIFIED_FLAG 16 
#define  CGI_BIN_FLAG 32


/*
 *	constants defined for extension
 *
 */
#define  NO_EXTENSION 0
#define  HTML_EXTENSION 1
#define  GIF_EXTENSION 2
#define  CGI_EXTENSION 3
#define  DATA_EXTENSION 4
#define  CLASS_EXTENSION 5
#define  MAP_EXTENSION  6
#define  JPEG_EXTENSION 7
#define  MPEG_EXTENSION 8
#define   OTHER_EXTENSION 9

#define   MAX_EXTENSIONS 9

typedef struct  _TEntry {
	u_4bytes			event_duration;
	u_4bytes			server_duration;
	u_4bytes			last_mod;
	u_4bytes			time_sec;
	u_4bytes			time_usec;

	u_4bytes			client;
	u_4bytes			server;
	u_4bytes			port;
	u_4bytes			path;
	u_4bytes			query;
	u_4bytes			size;

	unsigned short			status;
	unsigned  char			type;
	unsigned  char			flags;
 	method_t				method;
	protocol_t				protocol;
	}   TEntry;


#endif









