#
# Copyright (c) 1996 Regents of the University of California.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
# 	This product includes software developed by the MASH Research
# 	Group at the University of California Berkeley.
# 4. Neither the name of the University nor of the Research Group may be
#    used to endorse or promote products derived from this software without
#    specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-link.tcl,v 1.3 1997/01/27 01:16:24 mccanne Exp $
#
Class Link
Link instproc init { src dst } {
	$self next
	$self instvar trace_ fromNode_ toNode_
	set fromNode_ $src
	set toNode_ $dst
	set trace_ ""
}

Link instproc head {} {
	$self instvar head_
	return $head_
}

Link instproc queue {} {
	$self instvar queue_
	return $queue_
}

Class SimpleLink -superclass Link

SimpleLink instproc init { src dst bw delay q } {
	$self next $src $dst
	$self instvar link_ queue_ head_ toNode_
	set queue_ $q
	set link_ [new Delay/Link]
	$link_ set bandwidth_ $bw
	$link_ set delay_ $delay

	$queue_ target $link_
	$link_ target [$toNode_ entry]
	set head_ $queue_
}

#
# Build trace objects for this link and
# update the object linkage
#
SimpleLink instproc trace { ns f } {
	$self instvar enqT_ deqT_ drpT_ queue_ link_ head_ fromNode_ toNode_
	set enqT_ [$ns create-trace Enque $f $fromNode_ $toNode_]
	set deqT_ [$ns create-trace Deque $f $fromNode_ $toNode_]
	set drpT_ [$ns create-trace Drop $f $fromNode_ $toNode_]
	$drpT_ target [$ns set nullAgent_]
	$enqT_ target $queue_
	$queue_ target $deqT_
	$queue_ drop-trace $drpT_
	$deqT_ target $link_
	set head_ $enqT_
}
