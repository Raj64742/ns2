#include <stdio.h>
#include <ctype.h>
#include "packet.h"

char* ptype[] = {
        PT_NAMES
};

void
printLine(char *s) {
	printf("%s\\n\\\n", s);
}

char *
lcase(char *s) {
	static char charbuf[512];
	char* to = charbuf;
	while (*to++ = tolower(*s++))
		/* NOTHING */;
	*to = '\0';
	return charbuf;
}

main() {
	int i;
	printLine("static const char code[] = \"");
	printLine("global ptype pvals");
	printLine("set ptype(error) -1");
	printLine("set pvals(-1) error");
	char strbuf[512];
	for (i = 0; i < PT_NTYPE; i++) {
		sprintf(strbuf, "set ptype(%s) %d", lcase(ptype[i]), i);
		printLine(strbuf);
		sprintf(strbuf, "set pvals(%d) %s", i, ptype[i]);
		printLine(strbuf);
	}
	printLine("proc ptype2val {str} {");
	printLine("global ptype");
	printLine("set str [string tolower $str]");
	printLine("if ![info exists ptype($str)] {");
	printLine("set str error");
	printLine("}");
	printLine("set ptype($str)");
	printLine("}");
	printLine("proc pval2type {val} {");
	printLine("global pvals");
	printLine("if ![info exists pvals($val)] {");
	printLine("set val -1");
	printLine("}");
	printLine("set pvals($val)");
	printLine("}");
	printf("\";\n");
	printf("#include \"tclcl.h\"\n");
	printf("EmbeddedTcl et_ns_ptypes(code);\n");
}
