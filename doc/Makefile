#
# $Id: Makefile,v 1.9 1999/10/21 18:00:51 haldar Exp $
#

# by default, just assume latex2html is in the user's path
LATEX2HTML=	latex2html
PRE_PROCESS=	pre-process.pl
POST_PROCESS=	post-process.pl
L2H_WORKING_DIR = ./l2h

LATEX=          latex
BIBTEX=         bibtex
DVIPS=          dvips
CP=		/bin/cp
RM=		/bin/rm

FILES= \
	agents.tex \
	applications.tex \
	basic.tex \
	delays.tex \
	dynamics.tex \
	emulation.tex \
	error_model.tex \
	everything.tex \
	lan.tex \
	links.tex \
	math.tex \
	multicast.tex \
	nodes.tex \
	otcl.tex \
	packet_format.tex \
	queue_mgmt.tex \
	session.tex \
	sim.tex \
	srm.tex \
	tcp.tex \
	timers.tex \
	trace.tex \
	udp.tex \
	undocumented.tex \
	unicast.tex \
	webcache.tex \
	address.tex \
	hier-rtg.tex \
	mobility.tex \
	newnode.tex

all: everything.dvi everything.ps everything.html

everything.html: $(FILES)
	$(RM) -rf $(L2H_WORKING_DIR)
	mkdir $(L2H_WORKING_DIR)
	$(CP) -r *.sty *.tex *.aux *.bib *.bbl *.log figures $(L2H_WORKING_DIR)
	cd $(L2H_WORKING_DIR); 	$(PRE_PROCESS); \
		$(LATEX2HTML) everything.tex ; cd everything ; $(POST_PROCESS) ;
	$(RM) -rf $(L2H_WORKING_DIR)/everything/*.org
	$(CP) -rf $(L2H_WORKING_DIR)/everything/* .

everything.ps: $(FILES) everything.dvi
	dvips -f everything >everything.ps

everything.dvi: $(FILES)
	$(LATEX) everything
	@if egrep 'Rerun to get' everything.log; then echo 'Relatexing for cross-references.'; $(LATEX) everything; fi

gv: everything.ps $(FILES)
	gv everything.ps
