#
# Ported from CMU-Monarch project's mobility extensions -Padma, 10/98.
#



#Class ARPTable
# ======================================================================
#
# The ARPTable class
#
# ======================================================================
ARPTable instproc init args {
	eval $self next $args		;# parent class constructor
}

ARPTable set bandwidth_         0
ARPTable set delay_             5us
ARPTable set off_prune_         0
ARPTable set off_CtrMcast_      0
ARPTable set off_arp_           0

# ======================================================================
#
# The Node/MobileNode class
#
# ======================================================================


Node/MobileNode instproc init args {
	eval $self next $args		;# parent class constructor

	$self instvar nifs_ arptable_
	$self instvar netif_ mac_ ifq_ ll_
	$self instvar X_ Y_ Z_

	set X_ 0.0
	set Y_ 0.0
	set Z_ 0.0

#	set netif_	""		;# network interfaces
#	set mac_	""		;# MAC layers
#	set ifq_	""		;# interface queues
#	set ll_		""		;# link layers
        set arptable_ ""                ;# no ARP table yet

	set nifs_	0		;# number of network interfaces

}

Node/MobileNode instproc reset {} {
	$self instvar arptable_ nifs_
	$self instvar netif_ mac_ ifq_ ll_

        for {set i 0} {$i < $nifs_} {incr i} {
	    $netif_($i) reset
	    $mac_($i) reset
	    $ll_($i) reset
	    $ifq_($i) reset
	}

	if { $arptable_ != "" } {
	    $arptable_ reset 
	}
}


## replaced by base class Node methods

#Node/MobileNode instproc alloc-port {} {
#	$self instvar nports_ 
#	set p $nports_
#	incr nports_ 
#	return $p
#} 

#Node/MobileNode instproc entry {} {
#        $self instvar classifier_
#        return $classifier_
#}


#
# Attach an agent to a node.  Pick a port and
# bind the agent to the port number.
# if portnumber is 255, default target is set to the routing agent
#
Node/MobileNode instproc add-target {agent port } {

	global RouterTrace AgentTrace
	$self instvar dmux_
	
	$agent set sport_ $port

	if { $port == 255 } {			# non-routing agents
		#if { $RouterTrace == "ON" } {
			#
			# Send Target
			#
			#set sndT [cmu-trace Send "RTR" $self]
			#$sndT target [$self set ll_(0)]
			#$agent target $sndT
			
			#
			# Recv Target
			#
			#set rcvT [cmu-trace Recv "RTR" $self]
			#$rcvT target $agent
			#[$self set classifier_] defaulttarget $rcvT
			#$dmux_ install $port $rcvT
			
		#} else {
			#
			# Send Target
			#
			$agent target [$self set ll_(0)]

			#
			# Recv Target
			#
			[$self set classifier_] defaulttarget $agent
			$dmux_ install $port $agent
			#}

	} else {
		#if { $AgentTrace == "ON" } {
			#
			# Send Target
			#
			#set sndT [cmu-trace Send AGT $self]
			#$sndT target [$self entry]
			#$agent target $sndT

			#
			# Recv Target
			#
			#set rcvT [cmu-trace Recv AGT $self]
			#$rcvT target $agent
			#$dmux_ install $port $rcvT

		#} else {
			#
			# Send Target
			#
			$agent target [$self entry]

			#
			# Recv Target
			#
			$dmux_ install $port $agent
		#}
	}
}


#
#  The following setups up link layer, mac layer, network interface
#  and physical layer structures for the mobile node.
#
Node/MobileNode instproc add-interface { channel pmodel \
		lltype mactype qtype qlen iftype anttype} {

	$self instvar arptable_ nifs_
	$self instvar netif_ mac_ ifq_ ll_

	global ns_ MacTrace opt

	set t $nifs_
	incr nifs_

	set netif_($t)	[new $iftype]		;# interface
	set mac_($t)	[new $mactype]		;# mac layer
	set ifq_($t)	[new $qtype]		;# interface queue
	set ll_($t)	[new $lltype]		;# link layer
        set ant_($t)    [new $anttype]

	#
	# Local Variables
	#
	set nullAgent_ [$ns_ set nullAgent_]
	set netif $netif_($t)
	set mac $mac_($t)
	set ifq $ifq_($t)
	set ll $ll_($t)

	#
	# Initialize ARP table only once.
	#
	if { $arptable_ == "" } {
            set arptable_ [new ARPTable $self $mac]
            #set drpT [cmu-trace Drop "IFQ" $self]
            #$arptable_ drop-target $drpT
        }

	#
	# Link Layer
	#
	$ll arptable $arptable_
	$ll mac $mac
	$ll up-target [$self entry]
	$ll down-target $ifq

	#
	# Interface Queue
	#
	$ifq target $mac
	$ifq set qlim_ $qlen
	#set drpT [cmu-trace Drop "IFQ" $self]
	#$ifq drop-target $drpT

	#
	# Mac Layer
	#
	$mac netif $netif
	$mac up-target $ll
	$mac down-target $netif
	$mac nodes $opt(nn)

	#
	# Network Interface
	#
	$netif channel $channel
	$netif up-target $mac
	$netif propagation $pmodel	;# Propagation Model
	$netif node $self		;# Bind node <---> interface
	$netif antenna $ant_($t)

	#
	# Physical Channel
	#
	$channel addif $netif

	# ============================================================

	#if { $MacTrace == "ON" } {
		#
		# Trace RTS/CTS/ACK Packets
		#
		#set rcvT [cmu-trace Recv "MAC" $self]
		#$mac log-target $rcvT


		#
		# Trace Sent Packets
		#
		#set sndT [cmu-trace Send "MAC" $self]
		#$sndT target [$mac sendtarget]
		#$mac sendtarget $sndT

		#
		# Trace Received Packets
		#
		#set rcvT [cmu-trace Recv "MAC" $self]
		#$rcvT target [$mac recvtarget]
		#$mac recvtarget $rcvT

		#
		# Trace Dropped Packets
		#
		#set drpT [cmu-trace Drop "MAC" $self]
		#$mac drop-target $drpT
	#} else {
		$mac log-target [$ns_ set nullAgent_]
		$mac drop-target [$ns_ set nullAgent_]
	#}

	# ============================================================

	$self addif $netif
}

#
# Global Defaults - avoids those annoying warnings generated by bind()
#
Node/MobileNode set X_				0
Node/MobileNode set Y_				0
Node/MobileNode set Z_				0
Node/MobileNode set speed_				0
Node/MobileNode set position_update_interval_	0
Node/MobileNode set bandwidth_			0	;# not used
Node/MobileNode set delay_				0	;# not used
Node/MobileNode set off_prune_			0	;# not used
Node/MobileNode set off_CtrMcast_			0	;# not used



