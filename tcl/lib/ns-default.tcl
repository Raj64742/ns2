#
# Copyright (c) 1996-1997 Regents of the University of California.
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-default.tcl,v 1.13 1997/03/28 08:54:08 mccanne Exp $


#
#
# Set up all the default paramters.  Each default parameter
# is stored in the OTcl class template and copied from the
# class into the instance when the object is created
# (this happens in the Tcl/tcl-object.tcl helper library)
#

Trace set src_ 0
Trace set dst_ 0
Trace set callback_ 0

Agent set fid_ 0
Agent set prio_ 0
Agent set addr_ 0
Agent set dst_ 0
Agent set flags_ 0

##Agent set seqno_ 0 now is gone
##Agent set class_ 0 now is gone

Agent/TCP set seqno_ 0
Agent/TCP set t_seqno_ 0
Agent/TCP set maxburst_ 0
Agent/TCP set maxcwnd_ 0
Agent/TCP set window_ 20
Agent/TCP set windowInit_ 1
Agent/TCP set windowOption_ 1
Agent/TCP set windowConstant_ 4
Agent/TCP set windowThresh_ 0.002
Agent/TCP set overhead_ 0
Agent/TCP set ecn_ 0
Agent/TCP set packetSize_ 1000
Agent/TCP set bugFix_ true
Agent/TCP set tcpTick_ 0.1

Agent/TCP set dupacks_ 0
Agent/TCP set ack_ 0
Agent/TCP set cwnd_ 0
Agent/TCP set awnd_ 0
Agent/TCP set ssthresh_ 0
Agent/TCP set rtt_ 0
Agent/TCP set srtt_ 0
Agent/TCP set rttvar_ 0
Agent/TCP set backoff_ 0

Queue set limit_ 10

Queue/SFQ set maxqueue_ 40
Queue/SFQ set buckets_ 16

Queue/FQ set secsPerByte_ 0

Queue/RED set bytes_ false
Queue/RED set thresh_ 5
Queue/RED set maxthresh_ 15
Queue/RED set meanPacketSize_ 500
Queue/RED set ptc_ [expr 1.5e6 / (8. * 500)]
Queue/RED set queueWeight_ 0.002
Queue/RED set wait_ true
Queue/RED set linterm_ 50
Queue/RED set setbit_ false
Queue/RED set dropTail_ false
Queue/RED set doubleq_ false
Queue/RED set dqthresh_ 50

#XXX other kinds of sinks -> should reparent
Agent/TCPSink set packetSize_ 40
Agent/TCPSink set maxSackBlocks_ 3

Agent/TCPSink/DelAck set interval_ 100ms
Agent/TCPSink/Sack1/DelAck set interval_ 100ms

Agent/CBR set interval_ 3.75ms
Agent/CBR set random_ 0
Agent/CBR set packetSize_ 210

Agent/CBR/RTP set seqno_ 0
Agent/RTCP set seqno_ 0

Agent/Message set packetSize_ 180

Delay/Link set bandwidth_ 1.5Mb
Delay/Link set delay_ 100ms

Classifier/Addr set shift_ 12
Classifier/Addr set mask_ 0xffffffff

Classifier/Flow set shift_ 0
Classifier/Flow set mask_ 0xffffffff

Agent/LossMonitor set nlost_ 0
Agent/LossMonitor set npkts_ 0
Agent/LossMonitor set bytes_ 0
Agent/LossMonitor set lastPktTime_ 0
Agent/LossMonitor set expected_ 0

QueueMonitor set sum_ 0
QueueMonitor set lastx_ 0
QueueMonitor set lasty_ 0
QueueMonitor set size_ 0

Agent set class_ 0

Agent/TCPSimple set t_seqno_ 0
Agent/TCPSimple set dupacks_ 0
Agent/TCPSimple set packetSize_ 1000
Agent/TCPSimple set seqno_ 0
Agent/TCPSimple set ack_ 0
Agent/TCPSimple set cwnd_ 1
Agent/TCPSimple set window_ 20
