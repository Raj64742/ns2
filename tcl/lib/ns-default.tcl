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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-default.tcl,v 1.82 1997/12/20 00:41:35 heideman Exp $


#
#
# Set up all the default paramters.  Each default parameter
# is stored in the OTcl class template and copied from the
# class into the instance when the object is created
# (this happens in the Tcl/tcl-object.tcl helper library)
#

Trace set src_ -1
Trace set dst_ -1
Trace set callback_ 0
Trace set show_tcphdr_ 0

Agent set fid_ 0
Agent set prio_ 0
Agent set addr_ 0
Agent set dst_ 0
Agent set flags_ 0
Agent set ttl_ 32 ; # arbitrary choice here

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
Agent/TCP set timestamps_ false
Agent/TCP set slow_start_restart_ true
Agent/TCP set restart_bugfix_ true
Agent/TCP set tcpTick_ 0.1
Agent/TCP set maxrto_ 100000
Agent/TCP set srtt_init_ 0
Agent/TCP set rttvar_init_ 12
Agent/TCP set rtxcur_init_ 6.0
Agent/TCP instproc done {} { }

Agent/TCP set dupacks_ 0
Agent/TCP set ack_ 0
Agent/TCP set cwnd_ 0
Agent/TCP set awnd_ 0
Agent/TCP set ssthresh_ 0
Agent/TCP set rtt_ 0
Agent/TCP set srtt_ 0
Agent/TCP set rttvar_ 0
Agent/TCP set backoff_ 0
Agent/TCP set maxseq_ 0

Agent/TCP set ndatapack_ 0
Agent/TCP set ndatabytes_ 0
Agent/TCP set nackpack_ 0
Agent/TCP set nrexmit_ 0
Agent/TCP set nrexmitpack_ 0
Agent/TCP set nrexmitbytes_ 0
Agent/TCP set trace_all_oneline_ false

Agent/TCP/Fack set ss-div4_ false
Agent/TCP/Fack set rampdown_ false

# setting this to 1 implements some changes to reno 
# proposed by Janey Hoe (other than fixing reno's
# unnecessary retransmit timeouts)
Agent/TCP/Newreno set newreno_changes_ 0

Agent/TCP/Vegas set v_alpha_ 1
Agent/TCP/Vegas set v_beta_ 3
Agent/TCP/Vegas set v_gamma_ 1
Agent/TCP/Vegas set v_rtt_ 0

Agent/TCP/Vegas/RBP set rbp_scale_ 0.75
# rbp_rate_algorithm_'s are defined in tcp-rbp.cc.
# 1=RBP_VEGAS_RATE_ALGORITHM (default),
# 2=RBP_CWND_ALGORITHM
Agent/TCP/Vegas/RBP set rbp_rate_algorithm_ 1
Agent/TCP/Vegas/RBP set rbp_segs_actually_paced_ 0
Agent/TCP/Vegas/RBP set rbp_inter_pace_delay_ 0

Agent/TCP/Reno/RBP set rbp_scale_ 0.75
Agent/TCP/Reno/RBP set rbp_segs_actually_paced_ 0
Agent/TCP/Reno/RBP set rbp_inter_pace_delay_ 0
# Reno/RBP supports only RBP_CWND_ALGORITHM 
# Agent/TCP/Reno/RBP set rbp_rate_algorithm_ 2

Agent/TCP/Asym set g_ 0.125
Agent/TCP/Asym set damp_ 0

if [TclObject is-class Agent/TCP/FullTcp] {
	Agent/TCP/FullTcp set segsperack_ 1
	Agent/TCP/FullTcp set segsize_ 536
	Agent/TCP/FullTcp set tcprexmtthresh_ 3
	Agent/TCP/FullTcp set iss_ 0
	Agent/TCP/FullTcp set nodelay_ false
	Agent/TCP/FullTcp set data_on_syn_ false
	Agent/TCP/FullTcp set dupseg_fix_ true 
	Agent/TCP/FullTcp set dupack_reset_ false
	Agent/TCP/FullTcp set interval_ 0.1 ; # 100ms 
	Agent/TCP/FullTcp set close_on_empty_ false
	Agent/TCP/FullTcp set delay_growth_ false
}

Integrator set lastx_ 0.0
Integrator set lasty_ 0.0
Integrator set sum_ 0.0

# 10->50 to be like ns-1
Queue set limit_ 50
Queue set blocked_ false
Queue set unblock_on_resume_ true

Queue/SFQ set maxqueue_ 40
Queue/SFQ set buckets_ 16

Queue/FQ set secsPerByte_ 0

Queue set interleave_ false
Queue set acksfirst_ false
Queue set ackfromfront_ false

Queue/DropTail set drop-front_ false

Queue/RED set bytes_ false
Queue/RED set queue-in-bytes_ false
Queue/RED set thresh_ 5
Queue/RED set maxthresh_ 15
Queue/RED set mean_pktsize_ 500
Queue/RED set q_weight_ 0.002
Queue/RED set wait_ true
Queue/RED set linterm_ 10
Queue/RED set setbit_ false
Queue/RED set drop-tail_ true
Queue/RED set drop-front_ false
Queue/RED set drop-rand_ false
Queue/RED set doubleq_ false
Queue/RED set dqthresh_ 50
Queue/RED set ave_ 0.0
Queue/RED set prob1_ 0.0
Queue/RED set curq_ 0

Queue/DRR set buckets_ 10
Queue/DRR set blimit_ 25000
Queue/DRR set quantum_ 250
Queue/DRR set mask_ 0

Queue/CBQ set algorithm_ 0 ;# used by compat only, not bound
Queue/CBQ set maxpkt_ 1024
CBQClass set priority_ 0
CBQClass set level_ 1
CBQClass set extradelay_ 0.0
CBQClass set def_qtype_ DropTail
CBQClass set okborrow_ true
CBQClass set automaxidle_gain_ 0.9375

PacketQueue/Semantic set acksfirst_ false
PacketQueue/Semantic set filteracks_ false
PacketQueue/Semantic set replace_head_ false
PacketQueue/Semantic set priority_drop_ false
PacketQueue/Semantic set random_drop_ false
PacketQueue/Semantic set reconsAcks_ false
PacketQueue/Semantic set random_ecn_ false

#XXX other kinds of sinks -> should reparent
Agent/TCPSink set packetSize_ 40
Agent/TCPSink set maxSackBlocks_ 3

Agent/TCPSink/DelAck set interval_ 100ms
catch {
    Agent/TCPSink/Asym set interval_ 100ms
    Agent/TCPSink/Asym set maxdelack_ 5
}
Agent/TCPSink/Sack1/DelAck set interval_ 100ms

Agent/CBR set interval_ 3.75ms
Agent/CBR set random_ 0
Agent/CBR set packetSize_ 210
Agent/CBR set maxpkts_ 0x10000000

Agent/CBR/RTP set seqno_ 0
Agent/RTCP set seqno_ 0

Agent/Message set packetSize_ 180

DelayLink set bandwidth_ 1.5Mb
DelayLink set delay_ 100ms
DynamicLink set status_ 1

Classifier set offset_ 0
Classifier set shift_ 0
Classifier set mask_ 0xffffffff

Classifier/Addr set shift_ 12
Classifier/Addr set mask_ 0xffffffff

Classifier/Flow set shift_ 0
Classifier/Flow set mask_ 0xffffffff

Classifier/Hash set shift_ 0
Classifier/Hash set mask_ 0xffffffff
Classifier/Hash set default_ -1

Agent/LossMonitor set nlost_ 0
Agent/LossMonitor set npkts_ 0
Agent/LossMonitor set bytes_ 0
Agent/LossMonitor set lastPktTime_ 0
Agent/LossMonitor set expected_ 0

ErrorModel set errByte_ 0
ErrorModel set errTime_ 0
ErrorModel/Periodic set period_ 1.0
ErrorModel/Periodic set offset_ 0.0

QueueMonitor set size_ 0
QueueMonitor set pkts_ 0
QueueMonitor set parrivals_ 0
QueueMonitor set barrivals_ 0
QueueMonitor set pdepartures_ 0
QueueMonitor set bdepartures_ 0
QueueMonitor set pdrops_ 0
QueueMonitor set bdrops_ 0
QueueMonitor/ED set epdrops_ 0
QueueMonitor/ED set ebdrops_ 0
QueueMonitor/ED/Flowmon set enable_in_ true
QueueMonitor/ED/Flowmon set enable_out_ true
QueueMonitor/ED/Flowmon set enable_drop_ true
QueueMonitor/ED/Flowmon set enable_edrop_ true
QueueMonitor/ED/Flow set src_ -1
QueueMonitor/ED/Flow set dst_ -1
QueueMonitor/ED/Flow set flowid_ -1

Agent set class_ 0

Traffic/Expoo set burst-time .5
Traffic/Expoo set idle-time .5
Traffic/Expoo set rate 64Kb
Traffic/Expoo set packet-size 210

Traffic/Pareto set burst-time 500ms
Traffic/Pareto set idle-time 500ms
Traffic/Pareto set rate 64Kb
Traffic/Pareto set packet-size 210
Traffic/Pareto set shape 1.5

Agent/Mcast/Prune set packetSize_ 80

RandomVariable/Uniform set min_ 0.0
RandomVariable/Uniform set max_ 1.0
RandomVariable/Exponential set avg_ 1.0
RandomVariable/Pareto set avg_ 1.0
RandomVariable/Pareto set shape_ 1.5
RandomVariable/Constant set val_ 1.0
RandomVariable/HyperExponential set avg_ 1.0
RandomVariable/HyperExponential set cov_ 4.0

ErrorModel set rate_ 0.0
SelectErrorModel set rate_ 0.0                  ;# just to eliminate warnings
SRMErrorModel set rate_ 0.0                  ;# just to eliminate warnings

Source/Telnet set interval_ 1.0

networkinterface set intf_label_ -1

#
# The following are defautls for objects that are not necessarily TclObjects
#
Simulator set NumberInterfaces_ 0		;# to get intfs for mcast
Node set multiPath_ 0

Simulator set EnableMcast_ 0                    ;# to enable mcast
# Defaults for unicast addresses
# While changing these, ensure that the values are consistent in config.h
Simulator set NodeMask_ 0xffffff
Simulator set NodeShift_ 8
Simulator set PortMask_ 0xff

# Defaults for multicast addresses
Simulator set McastShift_ 15
Simulator set McastAddr_ 0x8000

# Dynamic routing defaults
rtObject set maxpref_   255
Agent/rtProto set preference_ 200		;# global default preference
Agent/rtProto/Direct set preference_ 100
Agent/rtProto/DV set preference_	120
Agent/rtProto/DV set INFINITY		 32
Agent/rtProto/DV set advertInterval	  2

rtModel set startTime_ 0.5
rtModel set finishTime_ "-"
rtModel/Exponential set upInterval_   10.0
rtModel/Exponential set downInterval_  1.0
rtModel/Deterministic set upInterval_   2.0
rtModel/Deterministic set downInterval_ 1.0

#
# SRM Agent defaults are in ../tcl/mcast/srm.tcl and ../mcast/srm-adaptive.tcl
#
