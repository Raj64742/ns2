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

if {![info exists ns]} {
	puts "Error:  sat-iridium-nodes.tcl is a supporting script for the "
	puts "        sat-iridium.tcl script-- run `sat-iridium.tcl' instead"
	exit
}

set plane 1
set n0 [$ns satnode-polar $alt $inc 0 0 $plane $linkargs $chan]
set n1 [$ns satnode-polar $alt $inc 0 32.73 $plane $linkargs $chan]
set n2 [$ns satnode-polar $alt $inc 0 65.45 $plane $linkargs $chan]
set n3 [$ns satnode-polar $alt $inc 0 98.18 $plane $linkargs $chan]
set n4 [$ns satnode-polar $alt $inc 0 130.91 $plane $linkargs $chan]
set n5 [$ns satnode-polar $alt $inc 0 163.64 $plane $linkargs $chan]
set n6 [$ns satnode-polar $alt $inc 0 196.36 $plane $linkargs $chan]
set n7 [$ns satnode-polar $alt $inc 0 229.09 $plane $linkargs $chan]
set n8 [$ns satnode-polar $alt $inc 0 261.82 $plane $linkargs $chan]
set n9 [$ns satnode-polar $alt $inc 0 294.55 $plane $linkargs $chan]
set n10 [$ns satnode-polar $alt $inc 0 327.27 $plane $linkargs $chan]

incr plane  
set n15 [$ns satnode-polar $alt $inc 31.6 16.36 $plane $linkargs $chan]
set n16 [$ns satnode-polar $alt $inc 31.6 49.09 $plane $linkargs $chan]
set n17 [$ns satnode-polar $alt $inc 31.6 81.82 $plane $linkargs $chan]
set n18 [$ns satnode-polar $alt $inc 31.6 114.55 $plane $linkargs $chan]
set n19 [$ns satnode-polar $alt $inc 31.6 147.27 $plane $linkargs $chan]
set n20 [$ns satnode-polar $alt $inc 31.6 180 $plane $linkargs $chan]
set n21 [$ns satnode-polar $alt $inc 31.6 212.73 $plane $linkargs $chan]
set n22 [$ns satnode-polar $alt $inc 31.6 245.45 $plane $linkargs $chan]
set n23 [$ns satnode-polar $alt $inc 31.6 278.18 $plane $linkargs $chan]
set n24 [$ns satnode-polar $alt $inc 31.6 310.91 $plane $linkargs $chan]
set n25 [$ns satnode-polar $alt $inc 31.6 343.64 $plane $linkargs $chan]

incr plane 
set n30 [$ns satnode-polar $alt $inc 63.2 0 $plane $linkargs $chan]
set n31 [$ns satnode-polar $alt $inc 63.2 32.73 $plane $linkargs $chan]
set n32 [$ns satnode-polar $alt $inc 63.2 65.45 $plane $linkargs $chan]
set n33 [$ns satnode-polar $alt $inc 63.2 98.18 $plane $linkargs $chan]
set n34 [$ns satnode-polar $alt $inc 63.2 130.91 $plane $linkargs $chan]
set n35 [$ns satnode-polar $alt $inc 63.2 163.64 $plane $linkargs $chan]
set n36 [$ns satnode-polar $alt $inc 63.2 196.36 $plane $linkargs $chan]
set n37 [$ns satnode-polar $alt $inc 63.2 229.09 $plane $linkargs $chan]
set n38 [$ns satnode-polar $alt $inc 63.2 261.82 $plane $linkargs $chan]
set n39 [$ns satnode-polar $alt $inc 63.2 294.55 $plane $linkargs $chan]
set n40 [$ns satnode-polar $alt $inc 63.2 327.27 $plane $linkargs $chan]

incr plane 
set n45 [$ns satnode-polar $alt $inc 94.8 16.36 $plane $linkargs $chan]
set n46 [$ns satnode-polar $alt $inc 94.8 49.09 $plane $linkargs $chan]
set n47 [$ns satnode-polar $alt $inc 94.8 81.82 $plane $linkargs $chan]
set n48 [$ns satnode-polar $alt $inc 94.8 114.55 $plane $linkargs $chan]
set n49 [$ns satnode-polar $alt $inc 94.8 147.27 $plane $linkargs $chan]
set n50 [$ns satnode-polar $alt $inc 94.8 180 $plane $linkargs $chan]
set n51 [$ns satnode-polar $alt $inc 94.8 212.73 $plane $linkargs $chan]
set n52 [$ns satnode-polar $alt $inc 94.8 245.45 $plane $linkargs $chan]
set n53 [$ns satnode-polar $alt $inc 94.8 278.18 $plane $linkargs $chan]
set n54 [$ns satnode-polar $alt $inc 94.8 310.91 $plane $linkargs $chan]
set n55 [$ns satnode-polar $alt $inc 94.8 343.64 $plane $linkargs $chan]

incr plane 
set n60 [$ns satnode-polar $alt $inc 126.4 0 $plane $linkargs $chan]
set n61 [$ns satnode-polar $alt $inc 126.4 32.73 $plane $linkargs $chan]
set n62 [$ns satnode-polar $alt $inc 126.4 65.45 $plane $linkargs $chan]
set n63 [$ns satnode-polar $alt $inc 126.4 98.18 $plane $linkargs $chan]
set n64 [$ns satnode-polar $alt $inc 126.4 130.91 $plane $linkargs $chan]
set n65 [$ns satnode-polar $alt $inc 126.4 163.64 $plane $linkargs $chan]
set n66 [$ns satnode-polar $alt $inc 126.4 196.36 $plane $linkargs $chan]
set n67 [$ns satnode-polar $alt $inc 126.4 229.09 $plane $linkargs $chan]
set n68 [$ns satnode-polar $alt $inc 126.4 261.82 $plane $linkargs $chan]
set n69 [$ns satnode-polar $alt $inc 126.4 294.55 $plane $linkargs $chan]
set n70 [$ns satnode-polar $alt $inc 126.4 327.27 $plane $linkargs $chan]

incr plane
set n75 [$ns satnode-polar $alt $inc 158 16.36 $plane $linkargs $chan]
set n76 [$ns satnode-polar $alt $inc 158 49.09 $plane $linkargs $chan]
set n77 [$ns satnode-polar $alt $inc 158 81.82 $plane $linkargs $chan]
set n78 [$ns satnode-polar $alt $inc 158 114.55 $plane $linkargs $chan]
set n79 [$ns satnode-polar $alt $inc 158 147.27 $plane $linkargs $chan]
set n80 [$ns satnode-polar $alt $inc 158 180 $plane $linkargs $chan]
set n81 [$ns satnode-polar $alt $inc 158 212.73 $plane $linkargs $chan]
set n82 [$ns satnode-polar $alt $inc 158 245.45 $plane $linkargs $chan]
set n83 [$ns satnode-polar $alt $inc 158 278.18 $plane $linkargs $chan]
set n84 [$ns satnode-polar $alt $inc 158 310.91 $plane $linkargs $chan]
set n85 [$ns satnode-polar $alt $inc 158 343.64 $plane $linkargs $chan]

# By setting the next_ variable on polar sats; handoffs can be optimized

$n0 set_next $n10; $n1 set_next $n0; $n2 set_next $n1; $n3 set_next $n2
$n4 set_next $n3; $n5 set_next $n4; $n6 set_next $n5; $n7 set_next $n6
$n8 set_next $n7; $n9 set_next $n8; $n10 set_next $n9

$n15 set_next $n25; $n16 set_next $n15; $n17 set_next $n16; $n18 set_next $n17
$n19 set_next $n18; $n20 set_next $n19; $n21 set_next $n20; $n22 set_next $n21
$n23 set_next $n22; $n24 set_next $n23; $n25 set_next $n24

$n30 set_next $n40; $n31 set_next $n30; $n32 set_next $n31; $n33 set_next $n32
$n34 set_next $n33; $n35 set_next $n34; $n36 set_next $n35; $n37 set_next $n36
$n38 set_next $n37; $n39 set_next $n38; $n40 set_next $n39

$n45 set_next $n55; $n46 set_next $n45; $n47 set_next $n46; $n48 set_next $n47
$n49 set_next $n48; $n50 set_next $n49; $n51 set_next $n50; $n52 set_next $n51
$n53 set_next $n52; $n54 set_next $n53; $n55 set_next $n54

$n60 set_next $n70; $n61 set_next $n60; $n62 set_next $n61; $n63 set_next $n62
$n64 set_next $n63; $n65 set_next $n64; $n66 set_next $n65; $n67 set_next $n66
$n68 set_next $n67; $n69 set_next $n68; $n70 set_next $n69

$n75 set_next $n85; $n76 set_next $n75; $n77 set_next $n76; $n78 set_next $n77
$n79 set_next $n78; $n80 set_next $n79; $n81 set_next $n80; $n82 set_next $n81
$n83 set_next $n82; $n84 set_next $n83; $n85 set_next $n84

