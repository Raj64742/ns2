#
# $Id: Makefile,v 1.17 2000/07/12 22:58:23 johnh Exp $
#

# by default, just assume latex2html is in the user's path
LATEX2HTML=	latex2html
L2H_WORKING_DIR = ./l2h
WEB_TARGET_DIR = /nfs/web/isi.edu/htdocs/nsnam/ns/doc

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
	newnode.tex \
	debug.tex \
	namtrace.tex \
	energymodel.tex

all: everything.dvi everything.ps.gz everything.html everything.pdf

html-pre-process:
	$(RM) -rf $(L2H_WORKING_DIR)
	mkdir $(L2H_WORKING_DIR)
	$(CP) -r *.sty *.tex *.aux *.bib *.bbl *.log figures $(L2H_WORKING_DIR)
	cd $(L2H_WORKING_DIR); 	../pre-process.pl .

html-process:
	cd $(L2H_WORKING_DIR); $(LATEX2HTML) -show_section_numbers everything.tex

html-post-process:
	cd $(L2H_WORKING_DIR); ../post-process.pl .

everything.html: html-pre-process html-process html-post-process
	@echo html results are in  $(L2H_WORKING_DIR)/everything


everything.ps: $(FILES) everything.dvi
	dvips -f everything >everything.ps

everything.ps.gz: $(FILES) everything.dvi everything.ps
	gzip < everything.ps >everything.ps.gz

everything.pdf: $(FILES) everything.ps everything.pdf
	ps2pdf everything.ps

everything.dvi: $(FILES)
	$(LATEX) everything
	@if egrep 'Rerun to get' everything.log; then echo 'Relatexing for cross-references.'; $(LATEX) everything; fi

gv: everything.ps $(FILES)
	gv everything.ps

ns-doc-auto: everything.ps.gz everything.html everything.pdf
	cp everything.ps.gz everything.pdf  $(L2H_WORKING_DIR)/everything/*.{html,css,gif} $(WEB_TARGET_DIR)


