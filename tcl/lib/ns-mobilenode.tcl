#
# Copyright (c) 1996-1998 Regents of the University of California.
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
# @(#) $Header: 
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
    $self instvar nifs_ arptable_
    $self instvar netif_ mac_ ifq_ ll_
    $self instvar X_ Y_ Z_
    $self instvar address_

    if {[llength $args] != 0} {
	set address_ $args
	set args [lreplace $args 0 1]
    }
    eval $self next $args		;# parent class constructor
    
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



#
# Attach an agent to a node.  Pick a port and
# bind the agent to the port number.
# if portnumber is 255, default target is set to the routing agent
#
Node/MobileNode instproc add-target {agent port } {
    
    global RouterTrace AgentTrace
    $self instvar dmux_ classifier_
    
    $agent set sport_ $port
    
    if { $port == 255 } {			# non-routing agents
	if { $RouterTrace == "ON" } {
	    #
	    # Send Target
	    #
	    set sndT [cmu-trace Send "RTR" $self]
	    $sndT target [$self set ll_(0)]
	    $agent target $sndT
	    
	    #
	    # Recv Target
	    #
	    set rcvT [cmu-trace Recv "RTR" $self]
	    $rcvT target $agent
	    $classifier_ defaulttarget $rcvT
	    $dmux_ install $port $rcvT
	    
	} else {
	    #
	    # Send Target
	    #
	    $agent target [$self set ll_(0)]
		
	    #
	    # Recv Target
	    #
	    $classifier_ defaulttarget $agent
	    $dmux_ install $port $agent
	}
	
    } else {
	if { $AgentTrace == "ON" } {
	    #
	    # Send Target
	    #
	    set sndT [cmu-trace Send AGT $self]
	    $sndT target [$self entry]
	    $agent target $sndT
		
	    #
	    # Recv Target
	    #
	    set rcvT [cmu-trace Recv AGT $self]
	    $rcvT target $agent
	    $dmux_ install $port $rcvT
		
	} else {
	    #
	    # Send Target
	    #
	    $agent target [$self entry]
	    
	    #
	    # Recv Target
	    #
	    $dmux_ install $port $agent
	}
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
            set drpT [cmu-trace Drop "IFQ" $self]
            $arptable_ drop-target $drpT
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
	set drpT [cmu-trace Drop "IFQ" $self]
	$ifq drop-target $drpT

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

	if { $MacTrace == "ON" } {
	    #
	    # Trace RTS/CTS/ACK Packets
	    #
	    set rcvT [cmu-trace Recv "MAC" $self]
	    $mac log-target $rcvT


	    #
	    # Trace Sent Packets
	    #
	    set sndT [cmu-trace Send "MAC" $self]
	    $sndT target [$mac sendtarget]
	    $mac sendtarget $sndT

	    #
	    # Trace Received Packets
	    #
	    set rcvT [cmu-trace Recv "MAC" $self]
	    $rcvT target [$mac recvtarget]
	    $mac recvtarget $rcvT

	    #
	    # Trace Dropped Packets
	    #
	    set drpT [cmu-trace Drop "MAC" $self]
	    $mac drop-target $drpT
	} else {
	    $mac log-target [$ns_ set nullAgent_]
	    $mac drop-target [$ns_ set nullAgent_]
	}

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



