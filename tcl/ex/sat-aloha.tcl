#
# Copyright (c) 1999 Regents of the University of California.
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
#       This product includes software developed by the MASH Research
#       Group at the University of California Berkeley.
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
# Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
#
# Simple script with a geostationary bent-pipe (repeater) satellite and 
# one hundred terminals using unslotted Aloha random access.  The
# traffic sources consist of exponential traffic generators.  With the
# traffic rates configured below, approximately one tenth of the packets 
# collide and must be retransmitted (these collisions are not explicitly 
# seen in the tracefile).
# 

global ns
set ns [new Simulator]

# Global configuration parameters for Aloha (also settable in ns-sat.tcl)
Mac/Sat/UnslottedAloha set mean_backoff_ 1s ; # mean exponential backoff time(s)
Mac/Sat/UnslottedAloha set rtx_limit_ 3; # max number of retrans. attempted 
Mac/Sat/UnslottedAloha set send_timeout_ 270ms; # resend if send times out

global opt
set opt(chan)           Channel/Sat
set opt(bw_up)		2Mb
set opt(bw_down)	2Mb
set opt(phy)            Phy/Sat
set opt(mac)            Mac/Sat/UnslottedAloha
set opt(ifq)            Queue/DropTail
set opt(qlim)		50
set opt(ll)             LL/Sat

# XXX This tracing enabling must precede link and node creation 
set outfile [open out.tr w]
$ns trace-all $outfile

# Set up satellite and terrestrial nodes

# GEO satellite at 0 degrees longitude 
$ns node-config -satNodeType geo-repeater \
		-llType $opt(ll) \
		-ifqType $opt(ifq) \
		-ifqLen $opt(qlim) \
		-macType $opt(mac) \
		-phyType $opt(phy) \
		-channelType $opt(chan) \
		-downlinkBW $opt(bw_down)
set n1 [$ns node]
$n1 set-position 0


# Place 100 terminals at 100 different locations
$ns node-config -satNodeType terminal
set num_nodes		100
for {set a 1} {$a <= $num_nodes} {incr a} {
	set n($a) [$ns node]
	$n($a) set-position [expr -15 + $a * 0.3] [expr 15 - $a * 0.3]
	$n($a) add-gsl geo $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
  		$opt(phy) [$n1 set downlink_] [$n1 set uplink_]
}

for {set a 1} {$a <= $num_nodes} {incr a} {
	set b [expr int($a + (0.5 * $num_nodes))]
	if {$b > 100} {
		incr b -100
	}

	set udp($a) [new Agent/UDP]
	$ns attach-agent $n($a) $udp($a)
	set exp($a) [new Application/Traffic/Exponential]
	$exp($a) attach-agent $udp($a)
	$exp($a) set rate_ 1Kb

	set null($a) [new Agent/Null]
	$ns attach-agent $n($b) $null($a)

	$ns connect $udp($a) $null($a)
	$ns at 1.0 "$exp($a) start"
}

$ns trace-all-satlinks $outfile

# We use centralized routing
set satrouteobject_ [new SatRouteObject]
$satrouteobject_ compute_routes

$ns at 100.0 "finish"

proc finish {} {
	global ns outfile 
	$ns flush-trace
	close $outfile

	exit 0
}

$ns run

