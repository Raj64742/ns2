#
# $Id: Makefile,v 1.4 1999/04/07 17:23:00 salehi Exp $
#

LATEX2HTMLDIR=	.
LATEX2HTML=	$(LATEX2HTMLDIR)/latex2html
PRE_PROCESS=	./pre-process.pl
POST_PROCESS=	./post-process.pl

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
	hier-rtg.tex

all: everything.dvi everything.ps everything.html

everything.html: $(FILES)
	@$(RM) -rf /tmp/everything
	@mkdir /tmp/everything
	@$(CP) -r * /tmp/everything
	@cd /tmp/everything ; $(PRE_PROCESS) ; \
		$(LATEX2HTML) everything.tex >& /dev/null ; \
		cd everything ; $(POST_PROCESS) 
	@$(RM) -rf /tmp/everything/everything/*.org
	@$(CP) -rf /tmp/everything/everything/* .

everything.ps: $(FILES) everything.dvi
	dvips -f everything >everything.ps

everything.dvi: $(FILES)
	$(LATEX) everything
	@if egrep 'Rerun to get' everything.log; then echo 'Relatexing for cross-references.'; $(LATEX) everything; fi

gv: everything.ps $(FILES)
	gv everything.ps
