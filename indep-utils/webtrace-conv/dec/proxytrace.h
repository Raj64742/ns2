#ifndef _PROXYTRACE_H_
#define _PROXYTRACE_H_

#include <stdio.h>

typedef  unsigned int u_4bytes;       /* unsigned number - 4 bytes */
typedef  unsigned short int u_2bytes; /* unsigned number - 2 bytes */

typedef enum {
    METHOD_NONE= 0,
    METHOD_GET,
    METHOD_POST,
    METHOD_HEAD,
    METHOD_CONNECT
} method_t;

typedef enum {
    PROTO_NONE = 0,
    PROTO_HTTP,
    PROTO_FTP,
    PROTO_GOPHER,
    PROTO_WAIS,
    PROTO_CACHEOBJ,
    PROTO_MAX
} protocol_t;

typedef enum {
	EXT_NONE = 0,
	EXT_HTML,
	EXT_GIF,
	EXT_CGI,
	EXT_DATA,
	EXT_CLASS,
	EXT_MAP,
	EXT_JPEG,
	EXT_MPEG,
	EXT_OTHER
} type_t;

const char *MethodStr(int method);
const char *ProtocolStr(int protocol);
const char *ExtensionStr(int type);     /* the extension (e.g. ".html") */
const char *ExtensionTypeStr(int type); /* the type (e.g. "HTML") */

/* entry flags */

#define  NO_PATH_FLAG 1    /* path consists of only / */
#define  PORT_SPECIFIED_FLAG 2
#define  NULL_PATH_ADDED_FLAG 4
#define  QUERY_FOUND_FLAG 8
#define  EXTENSION_SPECIFIED_FLAG 16 
#define  CGI_BIN_FLAG 32

typedef struct _TEntryHeader {
	u_4bytes  event_duration;
	u_4bytes  server_duration;
	u_4bytes  last_mod;
	u_4bytes  time_sec;
	u_4bytes  time_usec;

	u_4bytes  client;
	u_4bytes  server;
	u_4bytes  port;
	u_4bytes  path;
	u_4bytes  query;
	u_4bytes  size;
} TEntryHeader;

typedef struct _TEntryTail {
	unsigned short  status;
	unsigned  char  type;
	unsigned  char  flags;
 	method_t        method;
	protocol_t      protocol;
} TEntryTail;

typedef struct _TEntry {
	TEntryHeader head;
	u_4bytes     url;    /* missing in v1 format */
	TEntryTail   tail;
} TEntry;

#define  TRACE_HEADER_SIZE 8192

size_t ReadHeader(FILE *in_file, void *userBuf);
size_t ReadEntry(FILE *in_file, TEntry *entry);
size_t ReadEntryV1(FILE *in_file, TEntry *entry);

struct URL {
	URL(int i, int sd, int sz) : access(1), id(i), sid(sd), size(sz) {}
	int access;	// access counts
	int id;
	int sid, size;
};

struct ReqLog {
	ReqLog() {}
	ReqLog(double t, unsigned int c, unsigned int s, unsigned int u) :
		time(t), cid(c), sid(s), url(u) {}
	double time;
	unsigned int cid, sid, url;
};

#endif // proxytrace.h

