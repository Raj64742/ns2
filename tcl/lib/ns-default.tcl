# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
# Time-stamp: <2000-09-13 13:48:04 haoboy>
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-default.tcl,v 1.220 2000/09/14 18:19:27 haoboy Exp $


#
#
# Set up all the default paramters.  Each default parameter
# is stored in the OTcl class template and copied from the
# class into the instance when the object is created
# (this happens in the Tcl/tcl-object.tcl helper library)
#

########################################################

# Increased Floating Point Precision
set tcl_precision 17

Connector set debug_ false
TTLChecker set debug_ false

Trace set src_ -1
Trace set dst_ -1
Trace set callback_ 0
Trace set show_tcphdr_ 0
Trace set debug_ false

CMUTrace set debug_ false

Scheduler/RealTime set maxslop_ 0.010; # max allowed slop b4 error (sec)

#
# Queues and associated
#
Integrator set lastx_ 0.0
Integrator set lasty_ 0.0
Integrator set sum_ 0.0

# 10->50 to be like ns-1
Queue set limit_ 50
Queue set blocked_ false
Queue set unblock_on_resume_ true

Queue set interleave_ false
Queue set acksfirst_ false
Queue set ackfromfront_ false
Queue set debug_ false

Queue/SFQ set maxqueue_ 40
Queue/SFQ set buckets_ 16

Queue/FQ set secsPerByte_ 0
# change DropTail to RED for RED on individual queues
FQLink set queueManagement_ DropTail

Queue/DropTail set drop_front_ false

Queue/DropTail/PriQueue set Prefer_Routing_Protocols    1

Queue/RED set bytes_ false
Queue/RED set queue_in_bytes_ false
Queue/RED set thresh_ 5
Queue/RED set maxthresh_ 15
Queue/RED set mean_pktsize_ 500
Queue/RED set q_weight_ 0.002
Queue/RED set wait_ true
Queue/RED set linterm_ 10
Queue/RED set setbit_ false
Queue/RED set gentle_ false
Queue/RED set drop_tail_ true
Queue/RED set drop_front_ false
Queue/RED set drop_rand_ false
Queue/RED set doubleq_ false
Queue/RED set ns1_compat_ false
Queue/RED set dqthresh_ 50
Queue/RED set ave_ 0.0
Queue/RED set prob1_ 0.0
Queue/RED set curq_ 0

Queue/RED/RIO set bytes_ false
Queue/RED/RIO set queue_in_bytes_ false
Queue/RED/RIO set thresh_ 5
Queue/RED/RIO set maxthresh_ 15
Queue/RED/RIO set in_thresh_ 15
Queue/RED/RIO set in_maxthresh_ 30
Queue/RED/RIO set out_thresh_ 5
Queue/RED/RIO set out_maxthresh_ 15
Queue/RED/RIO set mean_pktsize_ 500
Queue/RED/RIO set q_weight_ 0.002
Queue/RED/RIO set wait_ true
Queue/RED/RIO set linterm_ 10
Queue/RED/RIO set in_linterm_ 50
Queue/RED/RIO set out_linterm_ 5
Queue/RED/RIO set setbit_ false
Queue/RED/RIO set gentle_ false
Queue/RED/RIO set in_gentle_ false
Queue/RED/RIO set out_gentle_ false
Queue/RED/RIO set drop_tail_ true
Queue/RED/RIO set drop_front_ false
Queue/RED/RIO set drop_rand_ false
Queue/RED/RIO set doubleq_ false
Queue/RED/RIO set ns1_compat_ false
Queue/RED/RIO set dqthresh_ 50
Queue/RED/RIO set ave_ 0.0
Queue/RED/RIO set in_ave_ 0.0
Queue/RED/RIO set out_ave_ 0.0
Queue/RED/RIO set prob1_ 0.0
Queue/RED/RIO set in_prob1_ 0.0
Queue/RED/RIO set out_prob1_ 0.0
Queue/RED/RIO set curq_ 0
Queue/RED/RIO set priority_method_ 0

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
CBQClass set debug_ false

SnoopQueue/In set debug_ false
SnoopQueue/Out set debug_ false
SnoopQueue/Drop set debug_ false
SnoopQueue/EDrop set debug_ false
SnoopQueue/Tagger set debug_ false

PacketQueue/Semantic set acksfirst_ false
PacketQueue/Semantic set filteracks_ false
PacketQueue/Semantic set replace_head_ false
PacketQueue/Semantic set priority_drop_ false
PacketQueue/Semantic set random_drop_ false
PacketQueue/Semantic set reconsAcks_ false
PacketQueue/Semantic set random_ecn_ false

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
QueueMonitor/ED/Flow/TB set target_rate_ 128000 
QueueMonitor/ED/Flow/TB set bucket_depth_ 10000
QueueMonitor/ED/Flow/TB set tbucket_ 10000
QueueMonitor/ED/Flow/TSW set target_rate_ 0
QueueMonitor/ED/Flow/TSW set win_len_ 10
QueueMonitor/ED/Flow/TSW set wait_ true

DelayLink set bandwidth_ 1.5Mb
DelayLink set delay_ 100ms
DelayLink set debug_ false

DynamicLink set status_ 1
DynamicLink set debug_ false

Filter set debug_ false
Filter/Field set offset_ 0
Filter/Field set match_  -1

# these are assigned when created
Classifier set offset_ 0
Classifier set shift_ 0
Classifier set mask_ 0xffffffff
Classifier set debug_ false

Classifier/Hash set default_ -1; # none
Classifier/Replicator set ignore_ 0

# MPLS Classifier
Classifier/Addr/MPLS set ttl_   32
Classifier/Addr/MPLS set trace_mpls_ 0
Classifier/Addr/MPLS set label_ -1
Classifier/Addr/MPLS set enable_reroute_    0
Classifier/Addr/MPLS set reroute_option_ 0
Classifier/Addr/MPLS set control_driven_ 0
Classifier/Addr/MPLS set data_driven_ 0

#
# Error models
#

ErrorModule set debug_ false

ErrorModel set enable_ 1
ErrorModel set markecn_ false
ErrorModel set rate_ 0
ErrorModel set bandwidth_ 2Mb
ErrorModel set debug_ false

ErrorModel/Trace set good_ 123456789
ErrorModel/Trace set loss_ 0
ErrorModel/Periodic set period_ 1.0
ErrorModel/Periodic set offset_ 0.0
ErrorModel/Periodic set burstlen_ 0.0
ErrorModel/MultiState set curperiod_ 0.0
ErrorModel/MultiState set sttype_ pkt
ErrorModel/MultiState set texpired_ 0

SelectErrorModel set enable_ 1
SelectErrorModel set markecn_ false
SelectErrorModel set rate_ 0
SelectErrorModel set bandwidth_ 2Mb
SelectErrorModel set pkt_type_ 2
SelectErrorModel set drop_cycle_ 10
SelectErrorModel set drop_offset_ 1
SelectErrorModel set debug_ false
SRMErrorModel set enable_ 1
SRMErrorModel set markecn_ false
SRMErrorModel set rate_ 0
SRMErrorModel set bandwidth_ 2Mb
SRMErrorModel set pkt_type_ 2
SRMErrorModel set drop_cycle_ 10
SRMErrorModel set drop_offset_ 1
SRMErrorModel set debug_ false
#MrouteErrorModel set enable_ 1
#MrouteErrorModel set rate_ 0
#MrouteErrorModel set bandwidth_ 2Mb
#MrouteErrorModel set pkt_type_ 2
#MrouteErrorModel set drop_cycle_ 10
#MrouteErrorModel set drop_offset_ 1
#MrouteErrorModel set good_ 99999999
#MrouteErrorModel set loss_ 0

rtModel set startTime_ 0.5
rtModel set finishTime_ "-"
rtModel/Exponential set upInterval_   10.0
rtModel/Exponential set downInterval_  1.0
rtModel/Deterministic set upInterval_   2.0
rtModel/Deterministic set downInterval_ 1.0

#
# Application
#

Application/Traffic/CBR_PP set rate_ 448Kb ;# corresponds to interval of 3.75ms
Application/Traffic/CBR_PP set packetSize_ 210
Application/Traffic/CBR_PP set random_ 0
Application/Traffic/CBR_PP set maxpkts_ 268435456; # 0x10000000
Application/Traffic/CBR_PP set PBM_ 2

Application/Traffic/Exponential set burst_time_ .5
Application/Traffic/Exponential set idle_time_ .5
Application/Traffic/Exponential set rate_ 64Kb
Application/Traffic/Exponential set packetSize_ 210

Application/Traffic/Pareto set burst_time_ 500ms
Application/Traffic/Pareto set idle_time_ 500ms
Application/Traffic/Pareto set rate_ 64Kb
Application/Traffic/Pareto set packetSize_ 210
Application/Traffic/Pareto set shape_ 1.5

Application/Traffic/CBR set rate_ 448Kb	;# corresponds to interval of 3.75ms
Application/Traffic/CBR set packetSize_ 210
Application/Traffic/CBR set random_ 0
Application/Traffic/CBR set maxpkts_ 268435456; # 0x10000000

Application/Telnet set interval_ 1.0

RandomVariable/Uniform set min_ 0.0
RandomVariable/Uniform set max_ 1.0
RandomVariable/Exponential set avg_ 1.0
RandomVariable/Pareto set avg_ 1.0
RandomVariable/Pareto set shape_ 1.5
RandomVariable/ParetoII set avg_ 10.0
RandomVariable/ParetoII set shape_ 1.2
RandomVariable/Constant set val_ 1.0
RandomVariable/HyperExponential set avg_ 1.0
RandomVariable/HyperExponential set cov_ 4.0
RandomVariable/Empirical set minCDF_ 0
RandomVariable/Empirical set maxCDF_ 1
RandomVariable/Empirical set interpolation_ 0
RandomVariable/Empirical set maxEntry_ 32
RandomVariable/Normal set avg_ 0.0
RandomVariable/Normal set std_ 1.0
RandomVariable/LogNormal set avg_ 1.0
RandomVariable/LogNormal set std_ 1.0

ADC/MS set debug_ false
ADC/HB set debug_ false
ADC/Param set debug_ false
ADC/ACTP set debug_ false
ADC/ACTO set debug_ false

Est/Null set debug_ false
Est/TimeWindow set debug_ false
Est/ExpAvg set debug_ false
Est/PointSample set debug_ false

MeasureMod set debug_ false
SALink set debug_ false

#
# Node
#

Node set multiPath_ 0
Node set rtagent_port_ 255

Node/MobileNode set X_				0
Node/MobileNode set Y_				0
Node/MobileNode set Z_				0
Node/MobileNode set speed_				0
Node/MobileNode set position_update_interval_	0
Node/MobileNode set bandwidth_			0	;# not used
Node/MobileNode set delay_				0	;# not used
Node/MobileNode set REGAGENT_PORT 0
Node/MobileNode set DECAP_PORT 1

# Default settings for Hierarchical topology
#
# Bits are allocated for different fields like port, nodeid, mcast, 
# hierarchical-levels. 
# All Mask and Shift values are stored in Class AddrParams.
AddrParams set ALL_BITS_SET 0xffffffff
AddrParams PortShift 0
AddrParams PortMask [AddrParams set ALL_BITS_SET]
AddrParams set domain_num_ 1
AddrParams set def_clusters 4
AddrParams set def_nodes 5

####  Default and Maximum Address space - leaving the MSB as signed bit
AllocAddrBits set DEFADDRSIZE_ 32
AllocAddrBits set MAXADDRSIZE_ 32                ;# leaving the signed bit

Simulator set node_factory_ Node
Simulator set nsv1flag 0
Simulator set mobile_ip_ 0			 ;# flag for mobileIP
Simulator set routingAgent_ ""
Simulator set addressType_   ""
Simulator set MovementTrace_ OFF
# This flag should be initially empty. It will be set to either ON or OFF
# by Simulator::create-wireless-node{}. 
Simulator set IMEPFlag_ ""
Simulator set WirelessNewTrace_ 0

# Enable packet reference count
SessionSim set rc_ 0

# Defaults for multicast addresses
Simulator set McastBaseAddr_ 0x80000000
Simulator set McastAddr_ 0x80000000

# Default values used for wireless simulations
Simulator set AgentTrace_ ON
Simulator set RouterTrace_ OFF
Simulator set MacTrace_   OFF

SessionHelper set rc_ 0                      ;# just to eliminate warnings
SessionHelper set debug_ false

NetworkInterface set debug_ false

# SRM Agent defaults are in ../tcl/mcast/srm.tcl and ../mcast/srm-adaptive.tcl

# IntServ Object specific defaults are in ../tcl/lib/ns-intserv.tcl

# defaults for tbf
TBF set rate_ 64k
TBF set bucket_ 1024
TBF set qlen_ 0

#
# mobile Ip
#
MIPEncapsulator set addr_ 0
MIPEncapsulator set port_ 0
MIPEncapsulator set shift_ 0
MIPEncapsulator set mask_ [AddrParams set ALL_BITS_SET]
MIPEncapsulator set ttl_ 32
MIPEncapsulator set debug_ false
 
# HTTP-related defaults are in ../tcl/webcache/http-agent.tcl

#
# Wireless simulation support 
#

Mac set debug_ false
ARPTable set debug_ false
God set debug_ false

Mac/Tdma set slot_packet_len_	1500
Mac/Tdma set max_node_num_	64

LL set mindelay_                50us
LL set delay_                   25us
LL set bandwidth_               0       ;# not used
LL set debug_ false

# Unity gain, omni-directional antennas
# Set up the antennas to be centered in the node and 1.5 meters above it
Antenna/OmniAntenna set X_ 0
Antenna/OmniAntenna set Y_ 0
Antenna/OmniAntenna set Z_ 1.5 
Antenna/OmniAntenna set Gt_ 1.0
Antenna/OmniAntenna set Gr_ 1.0

# Initialize the SharedMedia interface with parameters to make
# it work like the 914MHz Lucent WaveLAN DSSS radio interface
Phy/WirelessPhy set CPThresh_ 10.0
Phy/WirelessPhy set CSThresh_ 1.559e-11
Phy/WirelessPhy set RXThresh_ 3.652e-10
Phy/WirelessPhy set bandwidth_ 2e6
Phy/WirelessPhy set Pt_ 0.28183815
Phy/WirelessPhy set freq_ 914e+6
Phy/WirelessPhy set L_ 1.0  
Phy/WirelessPhy set debug_ false

Phy/WiredPhy set bandwidth_ 10e6
Phy/WiredPhy set debug_ false
Phy/Repeater set debug_ false
LanRouter set debug_ false

Phy/Sat set debug_ false
Mac/Sat set debug_ false
LL/Sat set debug_ false

# Shadowing propagation model
Propagation/Shadowing set pathlossExp_ 2.0
Propagation/Shadowing set std_db_ 4.0
Propagation/Shadowing set dist0_ 1.0
Propagation/Shadowing set seed_ 0

# variables of the link state module is moved to ns-rtProtoLS.tcl

#
# Agents
#
Agent set fid_ 0
Agent set prio_ 0
Agent set agent_addr_ -1
Agent set agent_port_ -1
Agent set dst_addr_ -1
Agent set dst_port_ -1
Agent set flags_ 0
Agent set ttl_ 32 ; # arbitrary choice here
Agent set debug_ false
Agent set class_ 0

##Agent set seqno_ 0 now is gone
##Agent set class_ 0 now is gone

Agent/Ping set packetSize_ 64

Agent/UDP set packetSize_ 1000
Agent/UDP instproc done {} { }

Agent/TCP set seqno_ 0
Agent/TCP set t_seqno_ 0
Agent/TCP set maxburst_ 0
Agent/TCP set maxcwnd_ 0
Agent/TCP set numdupacks_ 3
Agent/TCP set window_ 20
Agent/TCP set windowInit_ 1
Agent/TCP set windowInitOption_ 1
Agent/TCP set syn_ false
Agent/TCP set windowOption_ 1
Agent/TCP set windowConstant_ 4
Agent/TCP set windowThresh_ 0.002
Agent/TCP set decrease_num_ 0.5
Agent/TCP set increase_num_ 1.0
Agent/TCP set k_parameter_ 0.0 ;	# for binomial congestion control
Agent/TCP set l_parameter_ 1.0 ;  	# for binomial congestion control
Agent/TCP set overhead_ 0
Agent/TCP set ecn_ 0
Agent/TCP set old_ecn_ 0
Agent/TCP set packetSize_ 1000
Agent/TCP set tcpip_base_hdr_size_ 40
Agent/TCP set bugFix_ true
Agent/TCP set timestamps_ false
Agent/TCP set slow_start_restart_ true
Agent/TCP set restart_bugfix_ true
Agent/TCP set tcpTick_ 0.1
Agent/TCP set maxrto_ 100000
Agent/TCP set srtt_init_ 0
Agent/TCP set rttvar_init_ 12
Agent/TCP set rtxcur_init_ 6.0
Agent/TCP set T_SRTT_BITS 3
Agent/TCP set T_RTTVAR_BITS 2
Agent/TCP set rttvar_exp_ 2
Agent/TCP instproc done {} { }
Agent/TCP set noFastRetrans_ false

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
Agent/TCP set singledup_ 0
Agent/TCP set precisionReduce_ false
Agent/TCP set oldCode_ false

Agent/TCP set ndatapack_ 0
Agent/TCP set ndatabytes_ 0
Agent/TCP set nackpack_ 0
Agent/TCP set nrexmit_ 0
Agent/TCP set nrexmitpack_ 0
Agent/TCP set nrexmitbytes_ 0
Agent/TCP set trace_all_oneline_ false

Agent/TCP set QOption_ 0 
Agent/TCP set EnblRTTCtr_ 0
Agent/TCP set control_increase_ 0

# XXX Generate nam trace or plain old text trace for variables. 
# When it's true, generate nam trace.
Agent/TCP set nam_tracevar_ false

Agent/TCP/Fack set ss-div4_ false
Agent/TCP/Fack set rampdown_ false

Agent/TCP set eln_ 0
Agent/TCP set eln_rxmit_thresh_ 1
Agent/TCP set delay_growth_ false

Agent/TCP set CoarseTimer_      0

Agent/TCPSink set sport_        0
Agent/TCPSink set dport_        0         

#XXX other kinds of sinks -> should reparent
Agent/TCPSink set packetSize_ 40
Agent/TCPSink set maxSackBlocks_ 3
Agent/TCPSink set ts_echo_bugfix_ false
Agent/TCPSink set generateDSacks_ false
Agent/TCPSink set RFC2581_immediate_ack_ true

Agent/TCPSink/DelAck set interval_ 100ms
catch {
	Agent/TCPSink/Asym set interval_ 100ms
	Agent/TCPSink/Asym set maxdelack_ 5
}
Agent/TCPSink/Sack1/DelAck set interval_ 100ms

# setting this to 1 implements some changes to reno 
# proposed by Janey Hoe (other than fixing reno's
# unnecessary retransmit timeouts)
Agent/TCP/Newreno set newreno_changes_ 0
# setting this to 1 allows the retransmit timer to expire for
# a window with many packet drops
Agent/TCP/Newreno set newreno_changes1_ 0
Agent/TCP/Newreno set partial_window_deflation_ 0
Agent/TCP/Newreno set exit_recovery_fix_ 0

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
Agent/TCP/Reno/Asym set g_ 0.125
Agent/TCP/Newreno/Asym set g_ 0.125

# RFC793eduTcp -- 19990820, fcela@acm.org
Agent/TCP/RFC793edu set add793expbackoff_  true 
Agent/TCP/RFC793edu set add793jacobsonrtt_ false
Agent/TCP/RFC793edu set add793fastrtx_     false
Agent/TCP/RFC793edu set add793slowstart_   false
Agent/TCP/RFC793edu set add793additiveinc_ false
Agent/TCP/RFC793edu set add793karnrtt_     true 
Agent/TCP/RFC793edu set rto_               60
Agent/TCP/RFC793edu set syn_               true
Agent/TCP/RFC793edu set add793exponinc_    false

Agent/TFRC set packetSize_ 1000 
Agent/TFRC set df_ 0.95 ;	# decay factor for accurate RTT estimate
Agent/TFRC set tcp_tick_ 0.1 ;	
Agent/TFRC set ndatapack_ 0 ;	# Number of packets sent
Agent/TFRC set srtt_init_ 0 ;	# Variables for tracking RTT	
Agent/TFRC set rttvar_init_ 12  
Agent/TFRC set rtxcur_init_ 6.0	
Agent/TFRC set rttvar_exp_ 2	
Agent/TFRC set T_SRTT_BITS 3	
Agent/TFRC set T_RTTVAR_BITS 2	
Agent/TFRC set InitRate_ 300 ;	# Initial send rate	
Agent/TFRC set overhead_ 0 ;	# If > 0, dither outgoing packets
Agent/TFRC set ssmult_ 2 ; 	# Rate of increase during slow-start:
Agent/TFRC set bval_ 1 ;	# Value of B for TCP formula
Agent/TFRC set ca_ 1 ; 	 	# Enable Sqrt(RTT) congestion avoidance
Agent/TFRC set printStatus_ 0 
Agent/TFRC set maxHeavyRounds_ 1; # Number of rounds for sending rate allowed
				  #  to be greater than twice receiving rate.
Agent/TFRC set conservative_ 0 ;  # Set to true for a conservative 
				  # response to heavy congestion.

Agent/TFRCSink set packetSize_ 40
Agent/TFRCSink set InitHistorySize_ 100000
Agent/TFRCSink set NumFeedback_ 1 
Agent/TFRCSink set AdjustHistoryAfterSS_ 1
Agent/TFRCSink set NumSamples_ -1
Agent/TFRCSink set discount_ 1;	# History Discounting
Agent/TFRCSink set printLoss_ 0
Agent/TFRCSink set smooth_ 1 ;	# smoother Average Loss Interval
Agent/TFRCSink set minlc_ 4
Agent/TFRCSink set algo_ 1 ;  	# 1: algo from sigcomm paper 2: ewma 
				# 3: fixed window
Agent/TFRCSink set maxint_ 1000 ;     # max loss interval history 
Agent/TFRCSink set history_ 0.75 ;    # loss history for EWMA

if [TclObject is-class Agent/TCP/FullTcp] {
	Agent/TCP/FullTcp set segsperack_ 1; # ACK frequency
	Agent/TCP/FullTcp set segsize_ 536; # segment size
	Agent/TCP/FullTcp set tcprexmtthresh_ 3; # num dupacks to enter recov
	Agent/TCP/FullTcp set iss_ 0; # Initial send seq#
	Agent/TCP/FullTcp set nodelay_ false; # Nagle disable?
	Agent/TCP/FullTcp set data_on_syn_ false; # allow data on 1st SYN?
	Agent/TCP/FullTcp set dupseg_fix_ true ; # no rexmt w/dup segs from peer
	Agent/TCP/FullTcp set dupack_reset_ false; # exit recov on ack < highest
	Agent/TCP/FullTcp set interval_ 0.1 ; # delayed ACK interval 100ms 
	Agent/TCP/FullTcp set close_on_empty_ false; # close conn if sent all
	Agent/TCP/FullTcp set ts_option_size_ 10; # in bytes
	Agent/TCP/FullTcp set reno_fastrecov_ true; # fast recov true by default
	Agent/TCP/FullTcp set pipectrl_ false; # use "pipe" ctrl
	Agent/TCP/FullTcp set open_cwnd_on_pack_ true; # ^ win on partial acks?
	Agent/TCP/FullTcp set halfclose_ false; # do simplex closes (shutdown)?

	Agent/TCP/FullTcp/Newreno set recov_maxburst_ 2; # max burst dur recov

	Agent/TCP/FullTcp/Sack set sack_block_size_ 8; # bytes in a SACK block
	Agent/TCP/FullTcp/Sack set sack_option_size_ 2; # bytes in opt hdr
	Agent/TCP/FullTcp/Sack set max_sack_blocks_ 3; # max # of sack blks
}

# Default values used by wireless simulations
Agent/Null set sport_           0
Agent/Null set dport_           0

Agent/CBR set sport_            0
Agent/CBR set dport_            0

# Http invalidation agent
Agent/HttpInval set inval_hdr_size_ 40

Agent/RTP set seqno_ 0
Agent/RTP set interval_ 3.75ms
Agent/RTP set random_ 0
Agent/RTP set packetSize_ 210
Agent/RTP set maxpkts_ 0x10000000
Agent/RTP instproc done {} { }

Agent/RTCP set seqno_ 0

Agent/Message set packetSize_ 180

Agent/LossMonitor set nlost_ 0
Agent/LossMonitor set npkts_ 0
Agent/LossMonitor set bytes_ 0
Agent/LossMonitor set lastPktTime_ 0
Agent/LossMonitor set expected_ 0

# RAP
Agent/RAP set packetSize_ 512
Agent/RAP set seqno_ 0
Agent/RAP set sessionLossCount_ 0
Agent/RAP set ipg_ 2.0
Agent/RAP set alpha_ 1.0
Agent/RAP set beta_ 0.5
Agent/RAP set srtt_ 2.0
Agent/RAP set variance_ 0.0
Agent/RAP set delta_ 0.5
Agent/RAP set mu_ 1.2
Agent/RAP set phi_ 4.0
Agent/RAP set timeout_ 2.0
Agent/RAP set overhead_ 0
Agent/RAP set useFineGrain_ 0
Agent/RAP set kfrtt_ 0.9
Agent/RAP set kxrtt_ 0.01
Agent/RAP set debugEnable_ 0
Agent/RAP set rap_base_hdr_size_ 44
Agent/RAP set dpthresh_ 50
Agent/RAP instproc done {} { }

# Routing protocol agents
Agent/Mcast/Control set packetSize_ 80

# Dynamic routing defaults
Agent/rtProto set preference_ 200		;# global default preference
Agent/rtProto/Direct set preference_ 100
Agent/rtProto/DV set preference_	120
Agent/rtProto/DV set INFINITY		 [Agent set ttl_]
Agent/rtProto/DV set advertInterval	  2

Agent/Encapsulator set status_ 1
Agent/Encapsulator set overhead_ 20

Agent/DSRAgent set sport_ 255
Agent/DSRAgent set dport_ 255

Agent/MIPBS set adSize_ 48
Agent/MIPBS set shift_ 0
Agent/MIPBS set mask_ [AddrParams set ALL_BITS_SET]
Agent/MIPBS set ad_lifetime_ 2
 
Agent/MIPMH set home_agent_ 0
Agent/MIPMH set rreqSize_ 52
Agent/MIPMH set reg_rtx_ 0.5
Agent/MIPMH set shift_ 0
Agent/MIPMH set mask_ [AddrParams set ALL_BITS_SET]
Agent/MIPMH set reg_lifetime_ 2
 
# Intitialization for directed diffusion : Chalermek
Agent/Diff_Sink set packetSize_ 512
Agent/Diff_Sink set interval_   0.5
Agent/Diff_Sink set random_     1
Agent/Diff_Sink set maxpkts_    10000
Agent/Diff_Sink set data_type_  0

# PLM contributed by Arnaud Legout at EURECOM
Agent/LossMonitor/PLM set flag_PP_ 0
Agent/LossMonitor/PLM set packet_time_PP_ 0
Agent/LossMonitor/PLM set fid_PP_ 0
Agent/LossMonitor/PLM set seqno_ 0

# MPLS Label Distribution Protocol agent
Agent/LDP set trace_ldp_ 0

