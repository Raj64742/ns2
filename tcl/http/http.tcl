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

Class Http -superclass NsObject

Http set srcType_ TcpFull	# type of source agent
Http set snkType_ ""		# type of sink agent: "" means same as srcType_
Http set phttp_ 0		# persistent http
Http set maxConnections_ 4	# maximum number of concurrent connections

# The following rv* parameters can take a value or a RandomVariable object
Http set rvThinkTime_ 3.0	# value for think time in seconds
Http set rvReqLen_ 256		# length of request in byte
Http set rvRepLen_ 4000		# length of reply in byte
Http set rvNumImg_ 4		# number of image per request
Http set rvImgLen_ 8000		# length of image in byte

Http srcType {val} { $self set srcType_ $val }
Http snkType {val} { $self set snkType_ $val }
Http phttp {val} { $self set phttp_ $val }
Http maxConnections {val} { $self set maxConnections_ $val }
Http rvThinkTime {{val ""}} { $self set rvThinkTime_ $val }
Http rvRequestLen {val} { $self set rvRequestLen_ $val }
Http rvReplyLen {val} { $self set rvReplyLen_ $val }
Http rvNumImg {val} { $self set rvNumImg_ $val }
Http rvImgLen {val} { $self set rvNumImg_ $val }

Http instproc init {ns client server args} {
	eval $self next $args
	$self instvar srcType_ snkType_ phttp_ maxConnections_
	$self instvar rvThinkTime_ rvReqLen_ rvRepLen_ rvNumImg_ rvImgLen_
	$self instvar ns_ client_ server_ agents_

	set ns_ $ns
	set client_(node) $client
	set server_(node) $server
	set phttp_ $phttp
	if {$phttp > 0} {
		set $maxConnections_ 1
	}

	for {set i 0} {$i <= $maxConnections_} {incr i} {
		set csrc [new Agent/$srcType_]
		set ssrc [new Agent/$srcType_]
		append agents_(source) "$csrc $ssrc"
		$csrc proc done "$self doneRequest $i"
		$ssrc proc done "$self doneReply $i"
		if {$sinkType_ != ""} {
			set csnk [new Agent/$snkType_]
			set ssnk [new Agent/$snkType_]
			append agents_(sink) "$csnk $ssnk"
		} else {
			set csnk $ssrc
			set ssnk $csrc
		}
		set client_($i) [createSource FTP $client $server $csrc $csnk]
		set server_($i) [createSource FTP $server $client $ssrc $ssnk]
	}
}

Http instproc createSource {type src dst sa da} {
	$self instvar ns_
	$sa attach-source $type
	$ns_ attach-agent $src $sa
	$ns_ attach-agent $dst $da
	$ns_ connect $sa $da
}

# Http::agents return the agents of type "source" or "sink"
Http instproc agents {{type source}} {
	$self instvar agents_
	return $agents_($type)
}

Http instproc value {rv} {
	if {"$rv" > 999999999} {
		return [$rv value]
	}
	return $rv
}


Http instproc start {} {
	$self instvar srcType_ snkType_ phttp_ maxConnections_
	$self instvar rvThinkTime_ rvReqLen_ rvRepLen_ rvNumImg_ rvImgLen_
	$self instvar ns_ client_ server_ agents_
	$self instvar tStart_ numReq_

	set tStart_ [$ns_ now]
	set numReq_ 0
	$client_(0) sendByte [$self value $rvRequestLen_]
}

Http instproc doneRequest {id} {
	$self instvar srcType_ snkType_ phttp_ maxConnections_
	$self instvar rvThinkTime_ rvReqLen_ rvRepLen_ rvNumImg_ rvImgLen_
	$self instvar ns_ client_ server_ agents_
	$self instvar tStart_ numReq_

	if {$id == 0} {
		set nbyte [$self value $rvReplyLen_]
	} else {
		set nbyte [$self value $rvImgLen_]
	}
	$server_($id) sendByte $nbyte
}

Http instproc doneReply {id} {
	$self instvar srcType_ snkType_ phttp_ maxConnections_
	$self instvar rvThinkTime_ rvReqLen_ rvRepLen_ rvNumImg_ rvImgLen_
	$self instvar ns_ client_ server_ agents_
	$self instvar tStart_ numReq_

	if {$id == 0} {
		set numReq_ [value $rvNumImg_]
		for {set i 0} {$numReq_ > 0} {decr numReq_} {
			set id [expr 1 + ($i % $maxConnections_)]
			if {[incr i] > $maxConnections_} break
			$client_($id) sendByte [$self value $rvRequestLen_]
		}
		return
	}

	if {$numReq_ <= 0} {
		$self donePage
	} else {
		$client_($id) sendByte [$self value $rvRequestLen_]
		decr numReq_
	}
}

Http instproc donePage {} {
	$self instvar srcType_ snkType_ phttp_ maxConnections_
	$self instvar rvThinkTime_ rvReqLen_ rvRepLen_ rvNumImg_ rvImgLen_
	$self instvar ns_ client_ server_ agents_
	$self instvar tStart_ numReq_

	puts -nonewline " [format " %.3f" [expr [$ns_ now] - $tStart_]]"
	set thinkTime [$self value $rvThinkTime_]
	$ns_ at [expr [$ns_ now] + thinkTime] "$self start"
}
