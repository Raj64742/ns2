# dsr.tcl
# $Id: dsr.tcl,v 1.1 1998/12/08 19:19:51 haldar Exp $

# ======================================================================
# Default Script Options
# ======================================================================

set opt(rt_port) 255
set opt(cc)      "off"            ;# have god check the caches for bad links?

Agent/DSRAgent set sport_ 255
Agent/DSRAgent set dport_ 255

# ======================================================================
# god cache monitoring

#source tcl/ex/timer.tcl
Class CacheTimer -superclass Timer
CacheTimer instproc timeout {} {
    global opt node_;
    $self instvar agent;
    $agent check-cache
    $self sched 1.0
}

proc checkcache {a} {
    global cachetimer ns_ ns

    set ns $ns_
    set cachetimer [new CacheTimer]
    $cachetimer set agent $a
    $cachetimer sched 1.0
}

# ======================================================================
Class SRNode -superclass Node/MobileNode

SRNode instproc init {args} {
    global opt ns_ tracefd RouterTrace
    $self instvar dsr_agent_ dmux_ entry_point_

    eval $self next $args	;# parent class constructor

    # puts "making dsragent for node [$self id]"
    set dsr_agent_ [new Agent/DSRAgent]
    $dsr_agent_ ip-addr [$self id]

    if { $RouterTrace == "ON" } {
	# Recv Target
	set rcvT [cmu-trace Recv "RTR" $self]
	$rcvT target $dsr_agent_
	set entry_point_ $rcvT	
    } else {
	# Recv Target
	set entry_point_ $dsr_agent
    }

    #
    # Drop Target (always on regardless of other tracing)
    #
    set drpT [cmu-trace Drop "RTR" $self]
    $dsr_agent_ drop-target $drpT

    #
    # Log Target
    #
    set T [new Trace/Generic]
    $T target [$ns_ set nullAgent_]
    $T attach $tracefd
    $T set src_ [$self id]
    $dsr_agent_ log-target $T

    $dsr_agent_ target $dmux_

    # packets to the DSR port should be dropped, since we've
    # already handled them in the DSRAgent at the entry.
    set nullAgent_ [$ns_ set nullAgent_]
    $dmux_ install $opt(rt_port) $nullAgent_

    # SRNodes don't use the IP addr classifier.  The DSRAgent should
    # be the entry point
    $self instvar classifier_
    set classifier_ "srnode made illegal use of classifier_"

}

SRNode instproc start-dsr {} {
    $self instvar dsr_agent_
    global opt;

    $dsr_agent_ startdsr
    if {$opt(cc) == "on"} {checkcache $dsr_agent_}
}

SRNode instproc entry {} {
        $self instvar entry_point_
        return $entry_point_
}



SRNode instproc add-interface {args} {
# args are expected to be of the form
# $chan $prop $tracefd $opt(ll) $opt(mac)
    global ns_ opt RouterTrace

    eval $self next $args

    $self instvar dsr_agent_ ll_ mac_ ifq_

    $dsr_agent_ mac-addr [$mac_(0) id]

    if { $RouterTrace == "ON" } {
	# Send Target
	set sndT [cmu-trace Send "RTR" $self]
	$sndT target $ll_(0)
	$dsr_agent_ add-ll $sndT $ifq_(0)
    } else {
	# Send Target
	$dsr_agent_ add-ll $ll_(0) $ifq_(0)
    }
    
    # setup promiscuous tap into mac layer
    $dsr_agent_ install-tap $mac_(0)

}

SRNode instproc reset args {
    $self instvar dsr_agent_
    eval $self next $args

    $dsr_agent_ reset
}

# ======================================================================

proc dsr-create-mobile-node { id } {
	global ns_ chan prop topo tracefd opt node_
	global chan prop tracefd topo opt

	set node_($id) [new SRNode]

	set node $node_($id)
	$node random-motion 0		;# disable random motion
	$node topography $topo

        # connect up the channel
        $node add-interface $chan $prop $opt(ll) $opt(mac)	\
	     $opt(ifq) $opt(ifqlen) $opt(netif) $opt(ant)

	#
	# This Trace Target is used to log changes in direction
	# and velocity for the mobile node and log actions of the DSR agent
	#
	set T [new Trace/Generic]
	$T target [$ns_ set nullAgent_]
	$T attach $tracefd
	$T set src_ $id
	$node log-target $T

        $ns_ at 0.0 "$node start-dsr"
}

