#
# Copyright (c) 1994-1997 Regents of the University of California.
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
#	This product includes software developed by the Computer Systems
#	Engineering Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
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

proc makelinks { net bw delay pairs } {
	global ns node
	foreach p $pairs {
		set src [lindex $p 0]
		set dst [lindex $p 1]
		set angle [lindex $p 2]

	        mklink $net $src $dst $bw $delay $angle
	}
}

proc nam_config {net} {
	foreach k "0 1 2 3 4 5 6 7 8 13 14" {
	        $net node $k circle
	}
	foreach k "9 10 11 12" {
	        $net node $k square
	}

	makelinks $net 1.5Mb 10ms {
		{ 9 0 right-up }
		{ 9 1 right }
		{ 9 2 right-down }
		{ 10 3 right-up }
		{ 10 4 right }
		{ 10 5 right-down }
		{ 11 6 right-up }
		{ 11 7 right }
		{ 11 8 right-down }
	}

	makelinks $net 1.5Mb 40ms {
		{ 12 9 right-up }
		{ 12 10 right }
		{ 12 11 right-down }
	}

	makelinks $net 1.5Mb 10ms {
		{ 13 12 down } 
	}

	makelinks $net 1.5Mb 50ms {
		{ 14 12 right }
	}

        $net queue 14 12 0.5
        $net queue 12 14 0.5
        $net queue 10 3 0.5

        $net color 1 red
        $net color 2 white
        $net color 3 blue
        $net color 4 yellow
        $net color 5 LightBlue

        # chart utilization from 1 to 2 width 5sec
        # chart avgutilization from 1 to 2 width 5sec
}
