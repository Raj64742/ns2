#
# Copyright (c) 1997 Regents of the University of California.
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
# Code contributed by Giao Nguyen, http://daedalus.cs.berkeley.edu/~gnguyen
#

Class Http -superclass InitObject

Http set srcType_ TCP/FullTcp
Http set snkType_ ""
Http set phttp_ 0
Http set maxConn_ 4

# The following rv* parameters can take a value or a RandomVariable object
Http set rvThinkTime_ 3.0
Http set rvReqLen_ 256
Http set rvRepLen_ 4000
Http set rvImgLen_ 8000
Http set rvNumImg_ 4

Http instproc srcType {val} { $self set srcType_ $val }
Http instproc snkType {val} { $self set snkType_ $val }
Http instproc phttp {val} { $self set phttp_ $val }
Http instproc maxConn {val} { $self set maxConn_ $val }
Http instproc rvThinkTime {{val ""}} { $self set rvThinkTime_ $val }
Http instproc rvReqLen {val} { $self set rvReqLen_ $val }
Http instproc rvRepLen {val} { $self set rvRepLen_ $val }
Http instproc rvImgLen {val} { $self set rvImgLen_ $val }
Http instproc rvNumImg {val} { $self set rvNumImg_ $val }

Http instproc init {ns client server args} {
	$self init-all-vars $class
	eval $self next $args
	$self instvar srcType_ snkType_
	$self instvar rvThinkTime_ rvReqLen_ rvRepLen_ rvNumImg_ rvImgLen_
	$self instvar phttp_ maxConn_ numImg_ numGet_ numPut_
	$self instvar ns_ agents_ client_ server_ tStart_

	set ns_ $ns
	set client_(node) $client
	set server_(node) $server
	if {$phttp_ > 0} {
		set $maxConn_ 1
	}

	for {set i 0} {$i <= $maxConn_} {incr i} {
		set csrc [new Agent/$srcType_]
		set ssrc [new Agent/$srcType_]
		lappend agents_(source) $csrc $ssrc
		$csrc proc done {} "$self doneRequest $i"
		$ssrc proc done {} "$self doneReply $i"
		if {$snkType_ != ""} {
			set csnk [new Agent/$snkType_]
			set ssnk [new Agent/$snkType_]
			lappend agents_(sink) $csnk $ssnk
		} else {
			set csnk $ssrc
			set ssnk $csrc
		}
		set client_($i) [$self newXfer FTP $client $server $csrc $csnk]
		set server_($i) [$self newXfer FTP $server $client $ssrc $ssnk]
	}
}

Http instproc newXfer {type src dst sa da} {
	$self instvar ns_
	$ns_ attach-agent $src $sa
	$ns_ attach-agent $dst $da
	$ns_ connect $sa $da
	return [$sa attach-source $type]
}

# Http::agents return the agents of type "source" or "sink"
Http instproc agents {{type source}} {
	$self instvar agents_
	return $agents_($type)
}

Http instproc value {rv} {
	if ![catch "$rv value" val] {
		return $val
	}
	return $rv
}

Http instproc integer {rv} {
	if ![catch "$rv value" val] {
		return [expr int($val)]
	}
	return $rv
}


Http instproc start {} {
	$self instvar rvThinkTime_ rvReqLen_ rvRepLen_ rvNumImg_ rvImgLen_
	$self instvar phttp_ maxConn_ numImg_ numGet_ numPut_
	$self instvar ns_ agents_ client_ server_ tStart_

	set tStart_ [$ns_ now]
	set numGet_ 0
	set numPut_ 0
	set len [$self value $rvReqLen_]
	$client_(0) produceByte $len
#	puts "Http $self starts at $tStart_"
#	puts "$self reqH$numGet_ [$client_(0) set agent_] $len"
}

Http instproc doneRequest {id} {
	$self instvar rvThinkTime_ rvReqLen_ rvRepLen_ rvNumImg_ rvImgLen_
	$self instvar phttp_ maxConn_ numImg_ numGet_ numPut_
	$self instvar ns_ agents_ client_ server_ tStart_

	if {$id == 0} {
		set len [$self value $rvRepLen_]
	} else {
		set len [$self value $rvImgLen_]
	}
	$server_($id) produceByte $len
#	puts "$self send$numPut_ [$server_(0) set agent_] $len"
}

Http instproc doneReply {id} {
	$self instvar rvThinkTime_ rvReqLen_ rvRepLen_ rvNumImg_ rvImgLen_
	$self instvar phttp_ maxConn_ numImg_ numGet_ numPut_
	$self instvar ns_ agents_ client_ server_ tStart_

	if {$id == 0} {
		set numOpen $maxConn_
		set numImg_ [$self integer $rvNumImg_]
	} else {
		set numOpen 1
		incr numPut_
	}
	if {$numPut_ == $numImg_} {
		$self donePage
		return
	}

	for {set i 0} {$i < $numOpen} {incr i} {
		if {[incr numGet_] > $numImg_} {
			break
		}
		set id [expr $id ? $id : 1 + ($i % $maxConn_)]
		set len [$self value $rvReqLen_]
		$client_($id) produceByte $len
#		puts "$self reqI$numGet_ [$client_($id) set agent_] $len"
	}
}

Http instproc donePage {} {
	$self instvar rvThinkTime_ rvReqLen_ rvRepLen_ rvNumImg_ rvImgLen_
	$self instvar phttp_ maxConn_ numImg_ numGet_ numPut_
	$self instvar ns_ agents_ client_ server_ tStart_

	set now [$ns_ now]
	set tt [$self value $rvThinkTime_]
	set out [format "%.3f %.0f %.3f" [expr $now - $tStart_] $numImg_ $tt]
#	puts "Http $self donePage $out"
	$ns_ at [expr $now + $tt] "$self start"
}
