
#include <stdio.h>
#include "../../../autoconf.h"
#ifdef STDC_HEADERS
// for exit()
#include <unistd.h>
#include <stdlib.h>
#endif /* STDC_HEADERS */

#include "proxytrace.h"

static 
const char *MethodStrings[] = {
    "NONE",
    "GET",
    "POST",
    "HEAD",
    "CONNECT",
    "BAD#"
};

#define MAX_METHODS (sizeof(MethodStrings)/sizeof(*MethodStrings)-1)

static
const char *ProtocolStrings[] = {
    "NONE",
    "http",
    "ftp",
    "gopher",
    "wais",
    "cache_object",
    "TOTAL",
    "BAD#"
};

#define MAX_PROTOCOLS (sizeof(ProtocolStrings)/sizeof(*ProtocolStrings)-1)

static
const char *ExtensionStrings[] = {
	"",
    ".html",
    ".gif",
    ".cgi",
	".data",
	".class",
	".map",
    ".jpeg",
    ".mpeg",
	".OTHER",
	".BAD#"
};
  
#define MAX_EXTENSIONS (sizeof(ExtensionStrings)/sizeof(*ExtensionStrings)-1)

const char *MethodStr(int method) {

	if (method < 0 || method > MAX_METHODS)
		method = MAX_METHODS;

	return MethodStrings[method];
}

const char *ProtocolStr(int protocol) {

	if (protocol < 0 || protocol > MAX_PROTOCOLS)
		protocol = MAX_PROTOCOLS;

	return ProtocolStrings[protocol];
}

const char *ExtensionStr(int type) {

	if (type < 0 || type > MAX_EXTENSIONS)
		type = MAX_EXTENSIONS;

	return ExtensionStrings[type];
}

const char *ExtensionTypeStr(int type) {

	return (type == 0) ? "NONE" : (ExtensionStr(type)+1);
}


size_t ReadHeader(FILE *in_file, void *userBuf) {
	static char defaultBuf[TRACE_HEADER_SIZE];

	void *buf = (userBuf) ? userBuf : defaultBuf;

	if (fread(buf, TRACE_HEADER_SIZE, 1, in_file) != 1) {
		fprintf(stderr, "%s:%d error reading file header (%d bytes)\n",
			__FILE__, __LINE__, TRACE_HEADER_SIZE);
		exit(-2);
	}

	return TRACE_HEADER_SIZE;
}

size_t ReadEntry(FILE *in_file, TEntry *entry) {

	size_t items_read = fread(entry, sizeof(TEntry), 1, in_file);

	if (items_read < 0 || items_read > 1) {
		fprintf(stderr, "%s:%d error reading trace entry (%u bytes)\n",
			__FILE__, __LINE__, (unsigned)sizeof(TEntry));
		exit(-2);
	}

	return items_read*sizeof(TEntry);
}

size_t ReadEntryV1(FILE *in_file, TEntry *entry) {

	size_t items_read = fread(&entry -> head, sizeof(TEntryHeader), 1, in_file);

	if (items_read == 1)
		items_read = fread(&entry -> tail, sizeof(TEntryTail), 1, in_file);

	if (items_read < 0 || items_read > 1) {
		fprintf(stderr, "%s:%d error reading v1 trace entry (%u bytes)\n",
			__FILE__, __LINE__, (unsigned)(sizeof(TEntryHeader)+sizeof(TEntryTail)));
		exit(-2);
	}

	entry -> url = 0; /* initialize unused field */

	return items_read*(sizeof(TEntryHeader)+sizeof(TEntryTail));
}
