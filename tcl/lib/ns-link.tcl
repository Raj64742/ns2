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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-link.tcl,v 1.2 1997/01/26 23:26:33 mccanne Exp $
#
Class Link
Link instproc init { src dst } {
	$self next
	$self instvar trace fromNode toNode
	set fromNode $src
	set toNode $dst
	set trace ""
}

Link instproc head {} {
	$self instvar head
	return $head
}

Link instproc queue {} {
	$self instvar queue
	return $queue
}

Class SimpleLink -superclass Link

SimpleLink instproc init { src dst bw delay q } {
	$self next $src $dst
	$self instvar link queue head toNode
	set queue $q
	set link [new Delay/Link]
	$link set bandwidth $bw
	$link set delay $delay

	$queue target $link
	$link target [$toNode entry]
	set head $queue
}

#
# Build trace objects for this link and
# update the object linkage
#
SimpleLink instproc trace { ns f } {
	$self instvar enqT deqT drpT queue link head fromNode toNode
	set enqT [$ns create-trace Enque $f $fromNode $toNode]
	set deqT [$ns create-trace Deque $f $fromNode $toNode]
	set drpT [$ns create-trace Drop $f $fromNode $toNode]
	$drpT target [$ns set nullAgent]
	$enqT target $queue
	$queue target $deqT
	$queue drop-trace $drpT
	$deqT target $link
	set head $enqT
}

