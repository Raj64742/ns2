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

#IT IS NOT ENCOURAGED TO SUBCLASSS MOBILNODE CLASS DEFINED IN THIS FILE

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
    $self instvar nodetype_

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
       
# MIP node processing
        $self makemip-New$nodetype_
        
}

Node/MobileNode instproc makemip-New {} {

}


Node/MobileNode instproc makemip-NewBase {} {
}

Node/MobileNode instproc makemip-NewMobile {} {
}

Node/MobileNode instproc makemip-NewMIPBS {} {

    $self instvar regagent_ encap_ decap_ agents_ address_ dmux_ id_

   if { $dmux_ == "" } {
       set dmux_ [new Classifier/Port/Reserve]
       $dmux_ set mask_ 0x7fffffff
       $dmux_ set shift_ 0
       
       if [Simulator set EnableHierRt_] {  
	   $self add-hroute $address_ $dmux_
       } else {
	   $self add-route $address_ $dmux_
       }
   } 
   
   set regagent_ [new Agent/MIPBS $self]
   

   $self attach $regagent_ [Node/MobileNode  set REGAGENT_PORT]

   $self attach-encap 
   $self attach-decap

}

Node/MobileNode instproc makemip-NewMIPMH {} {

    $self instvar regagent_ dmux_ address_
 
    if { $dmux_ == "" } {
	set dmux_ [new Classifier/Port/Reserve]
	$dmux_ set mask_ 0x7fffffff
	$dmux_ set shift_ 0
	
	if [Simulator set EnableHierRt_] {  
	    $self add-hroute $address_ $dmux_
	} else {
	    $self add-route $address_ $dmux_
	}
    } 
    
    set regagent_ [new Agent/MIPMH $self]
    $self attach $regagent_ [Node/MobileNode set REGAGENT_PORT]
    $regagent_ node $self

}

Node/MobileNode instproc attach-encap {} {

    $self instvar encap_ address_ 
    
    set encap_ [new MIPEncapsulator]

    set mask 0x7fffffff
    set shift 0
    if [Simulator set EnableHierRt_] {
	set nodeaddr [AddrParams set-hieraddr $address_]
    } else {
	set nodeaddr [expr ( $address_ &			\
		[AddrParams set NodeMask_(1)] ) <<	\
		[AddrParams set NodeShift_(1) ]]
    }
    $encap_ set addr_ [expr ( ~($mask << $shift) & $nodeaddr)]
    $encap_ set port_ 1
    $encap_ target [$self entry]
    $encap_ set node_ $self
    #$encap_ set mask_ [AddrParams set NodeMask_(1)]
    #$encap_ set shift_ [AddrParams set NodeShift_(1)]
}

Node/MobileNode instproc attach-decap {} {
    $self instvar decap_ dmux_ agents_
    
    set decap_ [new Classifier/Addr/MIPDecapsulator]
    lappend agents_ $decap_
    set mask 0x7fffffff
    set shift 0
    if {[expr [llength $agents_] - 1] > $mask} {
	error "\# of agents attached to node $self exceeds port-field length of $mask bits\n"
    }
    $dmux_ install [Node/MobileNode set DECAP_PORT] $decap_
    #$decap_ set mask_ [AddrParams set NodeMask_(1)]
    #$decap_ set shift_ [AddrParams set NodeShift_(1)]
    #$decap_ def-target [$self entry]
}


Node/MobileNode instproc reset {} {
	$self instvar arptable_ nifs_
	$self instvar netif_ mac_ ifq_ ll_
	$self instvar imep_

        for {set i 0} {$i < $nifs_} {incr i} {
	    $netif_($i) reset
	    $mac_($i) reset
	    $ll_($i) reset
	    $ifq_($i) reset
	    if { [info exists opt(imep)] && $opt(imep) == "ON" } { $imep_($i) reset }

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

    #global opt
    $self instvar dmux_ classifier_
    $self instvar imep_ toraDebug_
    $self instvar namtraceFile_ 
 
    set ns_ [Simulator instance]

    set newapi [$ns_ imep-support]
    $agent set sport_ $port

    #special processing for TORA/IMEP node
    set toraonly [string first "TORA" [$agent info class]] 

    if {$toraonly != -1 } {
	$agent if-queue [$self set ifq_(0)]    ;# ifq between LL and MAC
	 #
        # XXX: The routing protocol and the IMEP agents needs handles
        # to each other.
        #
        $agent imep-agent [$self set imep_(0)]
        [$self set imep_(0)] rtagent $agent

    }

    # speciall processing for AODV
    set aodvonly [string first "AODV" [$agent info class]] 
    if {$aodvonly != -1 } {
	$agent if-queue [$self set ifq_(0)]    ;# ifq between LL and MAC
    }
    
    if { $port == 255 } {			# routing agents

	if { [Simulator set RouterTrace_] == "ON" } {
	    #
	    # Send Target
	    #
	    
	    if {$newapi != ""} {
	        set sndT [$ns_ mobility-trace Send "RTR" $self]
	    } else {
		set sndT [cmu-trace Send "RTR" $self]
	    }
            if [info exists namtraceFile_] {
                $sndT namattach $namtraceFile_
            }
            if { $newapi == "ON" } {
                 $agent target $imep_(0)
                 $imep_(0) sendtarget $sndT

		 # second tracer to see the actual
                 # types of tora packets before imep packs them
                 #if { [info exists opt(debug)] && $opt(debug) == "ON" } {
		  if { [info exists toraDebug_] && $toraDebug_ == "ON"} {
                       set sndT2 [$ns_ mobility-trace Send "TRP" $self]
                       $sndT2 target $imep_(0)
                       $agent target $sndT2
                 }
             } else {  ;#  no IMEP
                  $agent target $sndT
             }
	    
	    $sndT target [$self set ll_(0)]
#	    $agent target $sndT
	    
	    #
	    # Recv Target
	    #
	     
	    if {$newapi != ""} {
	         set rcvT [$ns_ mobility-trace Recv "RTR" $self]
	    } else {
		 set rcvT [cmu-trace Recv "RTR" $self]
	    }
            if [info exists namtraceFile_] {
                $rcvT namattach $namtraceFile_
            }
            if {$newapi == "ON" } {
            #    puts "Hacked for tora20 runs!! No RTR revc trace"
                [$self set ll_(0)] up-target $imep_(0)
                $classifier_ defaulttarget $agent

                # need a second tracer to see the actual
                # types of tora packets after imep unpacks them
                #if { [info exists opt(debug)] && $opt(debug) == "ON" } {
		# no need to support any hier node

		if {[info exists toraDebug_] && $toraDebug_ == "ON" } {
		    set rcvT2 [$ns_ mobility-trace Recv "TRP" $self]
		    $rcvT2 target $agent
		    [$self set classifier_] defaulttarget $rcvT2
		}
		
             } else {
                 $rcvT target $agent

		 $self install-defaulttarget $rcvT

#                 [$self set classifier_] defaulttarget $rcvT
#		 $classifier_ defaulttarget $rcvT

                 $dmux_ install $port $rcvT
	     }

#	    $rcvT target $agent
#	    $classifier_ defaulttarget $rcvT
#	    $dmux_ install $port $rcvT
	    
	} else {

	    #
	    # Send Target
	    #
	    # if tora is used
            if { $newapi == "ON" } {
                 $agent target $imep_(0)

                 # $imep_(0) sendtarget $sndT

		 # second tracer to see the actual
                 # types of tora packets before imep packs them
                 #if { [info exists opt(debug)] && $opt(debug) == "ON" } {
		  if { [info exists toraDebug_] && $toraDebug_ == "ON"} {
                       set sndT2 [$ns_ mobility-trace Send "TRP" $self]
                       $sndT2 target $imep_(0)
                       $agent target $sndT2
                 }

		 $imep_(0) sendtarget [$self set ll_(0)]

             } else {  ;#  no IMEP
		 
		 $agent target [$self set ll_(0)]
                 # $agent target $sndT
             }    

	    #$agent target [$self set ll_(0)]
		
	    #
	    # Recv Target
	    #

            if {$newapi == "ON" } {
            #    puts "Hacked for tora20 runs!! No RTR revc trace"

                [$self set ll_(0)] up-target $imep_(0)
                $classifier_ defaulttarget $agent

                # need a second tracer to see the actual
                # types of tora packets after imep unpacks them
                #if { [info exists opt(debug)] && $opt(debug) == "ON" } 
		# no need to support any hier node

		if {[info exists toraDebug_] && $toraDebug_ == "ON" } {
		    set rcvT2 [$ns_ mobility-trace Recv "TRP" $self]
		    $rcvT2 target $agent
		    [$self set classifier_] defaulttarget $rcvT2
		}
	    } else {
	        $self install-defaulttarget $agent
	    
	        #$classifier_ defaulttarget $agent

	        $dmux_ install $port $agent
	    }
	}
	
    } else {

	if { [Simulator set AgentTrace_] == "ON" } {
	    #
	    # Send Target
	    #
	   
	    if {$newapi != ""} {
	        set sndT [$ns_ mobility-trace Send AGT $self]
		
	    } else {
		set sndT [cmu-trace Send AGT $self]
            }

            if [info exists namtraceFile_]  {
                $sndT namattach $namtraceFile_
            }

	    $sndT target [$self entry]
	    $agent target $sndT
		
	    #
	    # Recv Target
	    #
	    if {$newapi != ""} {
	        set rcvT [$ns_ mobility-trace Recv AGT $self]
	    } else {
		set rcvT [cmu-trace Recv AGT $self]
	    }

            if [info exists namtraceFile_]  {
	       $rcvT namattach $namtraceFile_
	    }

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

#Node/MobileNode instproc install-defaulttarget {rcvT} {
#    [$self set classifier_] defaulttarget $rcvT
#}

# set transmission power
Node/MobileNode instproc setPt { val } {
    $self instvar netif_
    $netif_(0) setTxPower $val
}

# set receiving power
Node/MobileNode instproc setPr { val } {
    $self instvar netif_
    $netif_(0) setRxPower $val
}

# set idle power -- Chalermek
Node/MobileNode instproc setPidle { val } {
    $self instvar netif_
    $netif_(0) setIdlePower $val
}

#
#  The following setups up link layer, mac layer, network interface
#  and physical layer structures for the mobile node.
#
Node/MobileNode instproc add-interface { channel pmodel \
		lltype mactype qtype qlen iftype anttype} {
	$self instvar arptable_ nifs_
	$self instvar netif_ mac_ ifq_ ll_
	$self instvar imep_
	$self instvar namtraceFile_
	
	#global ns_ opt
	#set MacTrace [Simulator set MacTrace_]

	set ns_ [Simulator instance]


	set imepflag [$ns_ imep-support]

	set t $nifs_
	incr nifs_

	set netif_($t)	[new $iftype]		;# interface
	set mac_($t)	[new $mactype]		;# mac layer
	set ifq_($t)	[new $qtype]		;# interface queue
	set ll_($t)	[new $lltype]		;# link layer
        set ant_($t)    [new $anttype]

        if {$imepflag == "ON" } {              ;# IMEP layer
            set imep_($t) [new Agent/IMEP [$self id]]
            set imep $imep_($t)

            set drpT [$ns_ mobility-trace Drop "RTR" $self]
            if [info exists namtraceFile_] {
                $drpT namattach $namtraceFile_
            }
            $imep drop-target $drpT
            $ns_ at 0.[$self id] "$imep_($t) start"     ;# start beacon timer
        }


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
            # FOR backward compatibility sake, hack only
	    
	    if {$imepflag != ""} {
                set drpT [$ns_ mobility-trace Drop "IFQ" $self]
	    } else {
		set drpT [cmu-trace Drop "IFQ" $self]
	    }
	    
	    $arptable_ drop-target $drpT

            if [info exists namtraceFile_] {
	       $drpT namattach $namtraceFile_
	    }

        }

	#
	# Link Layer
	#
	$ll arptable $arptable_
	$ll mac $mac
#	$ll up-target [$self entry]
	$ll down-target $ifq

	if {$imepflag == "ON" } {
	    $imep recvtarget [$self entry]
            $imep sendtarget $ll
            $ll up-target $imep
        } else {
            $ll up-target [$self entry]
	}



	#
	# Interface Queue
	#
	$ifq target $mac
	$ifq set qlim_ $qlen

	if {$imepflag != ""} {
	    set drpT [$ns_ mobility-trace Drop "IFQ" $self]
	} else {
	    set drpT [cmu-trace Drop "IFQ" $self]
        }
	$ifq drop-target $drpT

        if  [info exists namtraceFile_]  {
           $drpT namattach $namtraceFile_
	}
 
	#
	# Mac Layer
	#
	$mac netif $netif
	$mac up-target $ll
	$mac down-target $netif
	#$mac nodes $opt(nn)
	set god_ [God instance]
        if {$mactype == "Mac/802_11"} {
		$mac nodes [$god_ num_nodes]
	}

	#
	# Network Interface
	#
	$netif channel $channel
	$netif up-target $mac
	$netif propagation $pmodel	;# Propagation Model
	$netif node $self		;# Bind node <---> interface
	$netif antenna $ant_($t)

	#
	# Physical Channel`
	#
	$channel addif $netif

	# ============================================================

	if { [Simulator set MacTrace_] == "ON" } {
	    #
	    # Trace RTS/CTS/ACK Packets
	    #

	    if {$imepflag != ""} {
	        set rcvT [$ns_ mobility-trace Recv "MAC" $self]
	    } else {
		set rcvT [cmu-trace Recv "MAC" $self]
	    }
	    $mac log-target $rcvT
            if [info exists namtraceFile_] {
                $rcvT namattach $namtraceFile_
            }


	    #
	    # Trace Sent Packets
	    #
	    if {$imepflag != ""} {
	        set sndT [$ns_ mobility-trace Send "MAC" $self]
	    } else {
		set sndT [cmu-trace Send "MAC" $self]
	    }
	    $sndT target [$mac down-target]
	    $mac down-target $sndT
            if [info exists namtraceFile_] {
                $sndT namattach $namtraceFile_
            }

	    #
	    # Trace Received Packets
	    #

	    if {$imepflag != ""} {
		set rcvT [$ns_ mobility-trace Recv "MAC" $self]
	        
	    } else {
	        set rcvT [cmu-trace Recv "MAC" $self]
	    }
	    $rcvT target [$mac up-target]
	    $mac up-target $rcvT
            if [info exists namtraceFile_] {
                $rcvT namattach $namtraceFile_
            }

	    #
	    # Trace Dropped Packets
	    #
	    if {$imepflag != ""} {
	        set drpT [$ns_ mobility-trace Drop "MAC" $self]
	    } else {
		set drpT [cmu-trace Drop "MAC" $self]`
	    }
	    $mac drop-target $drpT
            if [info exists namtraceFile_] {
                $drpT namattach $namtraceFile_
            }
	} else {
	    $mac log-target [$ns_ set nullAgent_]
	    $mac drop-target [$ns_ set nullAgent_]
	}

	# ============================================================

	$self addif $netif
}

Node/MobileNode instproc nodetrace { tracefd } {
    set ns_ [Simulator instance]
    #
    # This Trace Target is used to log changes in direction
    # and velocity for the mobile node.
    #
    set T [new Trace/Generic]
    $T target [$ns_ set nullAgent_]
    $T attach $tracefd
    $T set src_ [$self id]
    $self log-target $T    

}

Node/MobileNode instproc agenttrace {tracefd} {
    $self instvar namtraceFile_
 
    set ns_ [Simulator instance]
    
    set ragent [$self set ragent_]
    

    #
    # Drop Target (always on regardless of other tracing)
    #
    set drpT [$ns_ mobility-trace Drop "RTR" $self]

    if [info exists namtraceFile_] {
       $drpT namattach $namtraceFile_
    }

    $ragent drop-target $drpT
    #
    # Log Target
    #
    set T [new Trace/Generic]
    $T target [$ns_ set nullAgent_]
    $T attach $tracefd
    $T set src_ [$self id]
    #$ragent log-target $T
    $ragent tracetarget $T

    #
    # XXX: let the IMEP agent use the same log target.
    #
    set imepflag [$ns_ imep-support]
    if {$imepflag == "ON"} {
       [$self set imep_(0)] log-target $T
    }

}

## method to remove an entry from the hier classifiers
Node/MobileNode instproc clear-hroute args {
    $self instvar classifiers_
    set a [split $args]
    set l [llength $a]
    $classifiers_($l) clear [lindex $a [expr $l-1]] 
}

Node/MobileNode instproc mip-call {ragent} {
    $self instvar regagent_

    if [info exists regagent_] {
        $regagent_ ragent $ragent
    }

}

Node instproc mip-call {ragent} {

}

#-----------------------------------------------------------------
Agent/DSRAgent set sport_ 255
Agent/DSRAgent set dport_ 255
Agent/DSRAgent set rt_port 255

Class SRNodeNew -superclass Node/MobileNode

SRNodeNew instproc init {args} {
	#global ns ns_ opt tracefd RouterTrace

	$self instvar dsr_agent_ dmux_ entry_point_ address_

        set ns_ [Simulator instance]

	eval $self next $args	;# parent class constructor

	if {$dmux_ == "" } {
		set dmux_ [new Classifier/Port]
		$dmux_ set mask_ [AddrParams set PortMask_]
		$dmux_ set shift_ [AddrParams set PortShift_]
		#
		# point the node's routing entry to itself
		# at the port demuxer (if there is one)
		#
		#if [Simulator set EnableHierRt_] {
		    #$self add-hroute $address_ $dmux_
		#} else {
		    #$self add-route $address_ $dmux_
		#}
	}
	# puts "making dsragent for node [$self id]"
	set dsr_agent_ [new Agent/DSRAgent]
	# setup address (supports hier-address) for dsragent

	$dsr_agent_ addr $address_
	$dsr_agent_ node $self
	if [Simulator set mobile_ip_] {
	    $dsr_agent_ port-dmux [$self set dmux_]
	}
	# set up IP address
	$self addr $address_
	
    if { [Simulator set RouterTrace_] == "ON" } {
	# Recv Target
	set rcvT [$ns_ mobility-trace Recv "RTR" $self]
        set namtraceFile_ [$ns_ set namtraceAllFile_]
        if {  $namtraceFile_ != "" } {
            $rcvT namattach $namtraceFile_
        }
	$rcvT target $dsr_agent_
	set entry_point_ $rcvT	
    } else {
	# Recv Target
	set entry_point_ $dsr_agent_
    }

    #
    # Drop Target (always on regardless of other tracing)
    #
    #set drpT [$ns_ mobility-trace Drop "RTR" $self]
    #$dsr_agent_ drop-target $drpT
    #
    # Log Target
    #

    #set tracefd [$ns_ get-ns-traceall]
    #if {$tracefd != "" } {
    #     set T [new Trace/Generic]
    #     $T target [$ns_ set nullAgent_]
    #     $T attach $tracefd
    #     $T set src_ [$self id]
    #     $dsr_agent_ log-target $T
    #}


    $self set ragent_ $dsr_agent_

    $dsr_agent_ target $dmux_

    # packets to the DSR port should be dropped, since we've
    # already handled them in the DSRAgent at the entry.

    set nullAgent_ [$ns_ set nullAgent_]
    $dmux_ install [Agent/DSRAgent set rt_port] $nullAgent_

    # SRNodeNews don't use the IP addr classifier.  The DSRAgent should
    # be the entry point
    $self instvar classifier_
    set classifier_ "srnode made illegal use of classifier_"
    
    #$ns_ at 0.0 "$node start-dsr"
    return $self
}




SRNodeNew instproc start-dsr {} {
    $self instvar dsr_agent_
    #global opt;

    $dsr_agent_ startdsr
    #if {$opt(cc) == "on"} {checkcache $dsr_agent_}
}

SRNodeNew instproc entry {} {
        $self instvar entry_point_
        return $entry_point_
}

SRNodeNew instproc add-interface {args} {

    set ns_ [Simulator instance]

    eval $self next $args

    $self instvar dsr_agent_ ll_ mac_ ifq_

    $dsr_agent_ mac-addr [$mac_(0) id]


    if { [Simulator set RouterTrace_] == "ON" } {
	# Send Target
	set sndT [$ns_ mobility-trace Send "RTR" $self]
        set namtraceFile_ [$ns_ set namtraceAllFile_]
        if {$namtraceFile_ != "" } {
            $sndT namattach $namtraceFile_
        }
	$sndT target $ll_(0)
	$dsr_agent_ add-ll $sndT $ifq_(0)
    } else {
	# Send Target
	$dsr_agent_ add-ll $ll_(0) $ifq_(0)
    }
    
    # setup promiscuous tap into mac layer
    $dsr_agent_ install-tap $mac_(0)

}

SRNodeNew instproc reset args {
    $self instvar dsr_agent_
    eval $self next $args

    $dsr_agent_ reset
}

#new base station node

Class BaseNode -superclass {HierNode Node/MobileNode}

BaseNode instproc init {args} {

    $self instvar address_
    $self next $args
    set address_ $args

}

BaseNode instproc install-defaulttarget {rcvT} {

    $self instvar classifiers_
    set level [AddrParams set hlevel_]
    for {set i 1} {$i <= $level} {incr i} {
		$classifiers_($i) defaulttarget $rcvT
#		$classifiers_($i) bcast-receiver $rcvT
    }
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






