# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
#  Time-stamp: <2000-08-29 11:19:50 haoboy>
# 
#  Copyright (c) 1997 by the University of Southern California
#  All rights reserved.
# 
#  Permission to use, copy, modify, and distribute this software and its
#  documentation in source and binary forms for non-commercial purposes
#  and without fee is hereby granted, provided that the above copyright
#  notice appear in all copies and that both the copyright notice and
#  this permission notice appear in supporting documentation. and that
#  any documentation, advertising materials, and other materials related
#  to such distribution and use acknowledge that the software was
#  developed by the University of Southern California, Information
#  Sciences Institute.  The name of the University may not be used to
#  endorse or promote products derived from this software without
#  specific prior written permission.
# 
#  THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
#  the suitability of this software for any purpose.  THIS SOFTWARE IS
#  PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
#  INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 
#  Other copyrights might apply to parts of this software and are so
#  noted when applicable.
# 
#  Original source contributed by Gaeil Ahn. See below.
#
#  $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/mpls/ns-mpls-node.tcl,v 1.1 2000/08/29 19:28:03 haoboy Exp $

###########################################################################
# Copyright (c) 2000 by Gaeil Ahn                                	  #
# Everyone is permitted to copy and distribute this software.		  #
# Please send mail to fog1@ce.cnu.ac.kr when you modify or distribute     #
# this sources.								  #
###########################################################################

#############################################################
#                                                           #
#     File: File for MPLS Node class                        #
#     Author: Gaeil Ahn (fog1@ce.cnu.ac.kr), Jan. 2000      #
#                                                           #
#############################################################

# XXX
# This is so DUMB to not to have defaults for these *MPLS instprocs!
# It's like writing C in a single huge C++ class veneer. 
#
# All of the following will be repackaged into a standalone MPLS plugin 
# module, and attached to each node where MPLS is turned on. Most instprocs
# in this file will remain unchanged, except the following five, which are
# used as interfaces with current node-config{} architecture.

Node instproc set-node-addressMPLS args {
	# This is additional initialization procedures. It's ridiculous to
	# put these stuff under something called 'set-node-address', but 
	# we don't have another hook, and it's not OTcl any more!
	$self instvar linked_ldpagents_ in_label_range_ out_label_range_
	set linked_ldpagents_ ""
	set in_label_range_  0
	set out_label_range_ 1000
}

Node instproc entry-NewMPLS {} {
	return [$self entry-New]
}

Node instproc add-target-NewMPLS { agent port } {
	$self add-target-New $agent $port
}

Node instproc do-resetMPLS {} {
	$self do-reset
}

Node instproc mk-default-classifierMPLS {} {
	$self instvar address_ classifier_ id_
        set classifier_ [new Classifier/Addr/MPLS]
        $classifier_ set mpls_node_ $self
	set address_ $id_
}

# Now we're done with those dumb configurations and we can finally do
# something useful! Phew!

Node instproc enable-data-driven {} {
	[$self set classifier_] cmd enable-data-driven
}

Node instproc enable-control-driven {} {
	[$self set classifier_] cmd enable-control-driven
}

Node instproc new-incoming-label {} {
        $self instvar in_label_range_
        incr in_label_range_ 
        return $in_label_range_
}

Node instproc new-outgoing-label {} {
        $self instvar out_label_range_
        incr out_label_range_ -1
        return $out_label_range_
}

Node instproc make-ldp {} {
	set ldp [new Agent/LDP]
	$self attach-ldp $ldp
	return $ldp
}

Node instproc attach-ldp { agent } {
	$self instvar linked_ldpagents_
	lappend linked_ldpagents_ $agent
	$self attach $agent
}

Node instproc detach-ldp { agent } {
	$self instvar linked_ldpagents_
	# remove agent from list
	set k [lsearch -exact $linked_ldpagents_ $agent]
	if { $k >= 0 } {
		set linked_ldpagents_ [lreplace $linked_ldpagents_ $k $k]
	}
	$self detach $agent
}

Node instproc exist-fec {fec phb} {
	$self instvar classifier_ 
        return [$classifier_ exist-fec $fec $phb]
}

Node instproc get-incoming-iface {fec lspid} {
	$self instvar classifier_ 
        return [$classifier_ GetInIface $fec $lspid]
}

Node instproc get-incoming-label {fec lspid} {
	$self instvar classifier_ 
        return [$classifier_ GetInLabel $fec $lspid]
}

Node instproc get-outgoing-label {fec lspid} {
	$self instvar classifier_ 
        return [$classifier_ GetOutLabel $fec $lspid]
}

Node instproc get-outgoing-iface {fec lspid} {
	$self instvar classifier_ 
        return [$classifier_ GetOutIface $fec $lspid]
}

Node instproc get-fec-for-lspid {lspid} {
	$self instvar classifier_ 
        return [$classifier_ get-fec-for-lspid $lspid]
}

Node instproc in-label-install {fec lspid iface label} {
	$self instvar classifier_
	set dontcare [Classifier/Addr/MPLS dont-care]
	$self label-install $fec $lspid $iface $label $dontcare $dontcare
}

Node instproc out-label-install {fec lspid iface label} {
	$self instvar classifier_
	set dontcare [Classifier/Addr/MPLS dont-care]
	$self label-install $fec $lspid $dontcare $dontcare $iface $label
}

Node instproc in-label-clear {fec lspid} {
	$self instvar classifier_ 
	set dontcare [Classifier/Addr/MPLS dont-care]
	$self label-clear $fec $lspid -1 -1 $dontcare $dontcare
}

Node instproc out-label-clear {fec lspid} {
	$self instvar classifier_ 
	set dontcare [Classifier/Addr/MPLS dont-care]
	$self label-clear $fec $lspid $dontcare $dontcare -1 -1
}

Node instproc label-install {fec lspid iif ilbl oif olbl} {
	$self instvar classifier_ 
        $classifier_ LSPsetup $fec $lspid $iif $ilbl $oif $olbl
}

Node instproc label-clear {fec lspid iif ilbl oif olbl} {
        $self instvar classifier_ 
        $classifier_ LSPrelease $fec $lspid $iif $ilbl $oif $olbl
}

Node instproc flow-erlsp-install {fec phb lspid} {
        $self instvar classifier_ 
        $classifier_ ErLspBinding $fec $phb $lspid
}

Node instproc erlsp-stacking {erlspid tunnelid} {
        $self instvar classifier_ 
        $classifier_ ErLspStacking -1 $erlspid -1 $tunnelid
}

Node instproc flow-aggregation {fineFec finePhb coarseFec coarsePhb} {
        $self instvar classifier_ 
        $classifier_ FlowAggregation $fineFec $finePhb $coarseFec $coarsePhb
}

Node instproc enable-reroute {option} {
        $self instvar classifier_ 
        $classifier_ set enable_reroute_ 1
        $classifier_ set reroute_option_ 0
        if {$option == "drop"} {
		$classifier_ set reroute_option_ 0
        }
        if {$option == "L3"} {
		$classifier_ set reroute_option_ 1
        }
        if {$option == "new"} {
		$classifier_ set reroute_option_ 2
        }
}

Node instproc reroute-binding {fec phb lspid} {
        $self instvar classifier_ 
        $classifier_ aPathBinding $fec $phb -1 $lspid
}

Node instproc lookup-nexthop {node fec} {
        set ns [Simulator instance]
        set routingtable [$ns get-routelogic]
        set nexthop [$routingtable lookup $node $fec]
        return $nexthop
}

Node instproc get-nexthop {fec} {
	# find a next hop for fec
        set nodeid [$self id]
        set nexthop [$self lookup-nexthop $nodeid $fec]
        return $nexthop
}

Node instproc get-link-status {hop} {
	if {$hop < 0} {
		return "down"
	}
	set nexthop [$self get-nexthop $hop]
	if {$nexthop == $hop} {
		set status "up"
	} else {
		set status "down"
	}
	return $status
}

Node instproc is-egress-lsr { fec } {
        # Determine whether I am a egress-lsr for fec. 
        if { [$self id] == $fec } {
		return  "1"
        }
        set ns [Simulator instance]
        set nexthopid [$self get-nexthop $fec]
        if { $nexthopid < 0 } {
		return "-1"
        }
        set nexthop [$ns set Node_($nexthopid)]
	if { [$nexthop node-type] != "MPLS" } {
		return  "1"
        } else {
		return  "-1"
        }
}

Node instproc get-ldp-agents {} {
	# find ldp agents attached to this node
	$self instvar linked_ldpagents_
	return $linked_ldpagents_
}

Node instproc exist-ldp-agent { dst } {
	# determine whether or not a ldp agent attached to a dst node exists
	$self instvar linked_ldpagents_
	for {set i 0} {$i < [llength $linked_ldpagents_]} {incr i} {
		set ldpagent [lindex $linked_ldpagents_ $i]
		if { [$ldpagent peer-ldpnode] == $dst } {
			return "1"
		}
	}
	return "0"
}

Node instproc get-ldp-agent { dst } {
	# find a ldp agent attached to a dst
	$self instvar linked_ldpagents_
	for {set i 0} {$i < [llength $linked_ldpagents_]} {incr i} {
		set ldpagent [lindex $linked_ldpagents_ $i]
		if { [$ldpagent peer-ldpnode] == $dst } {
			return $ldpagent
		}
	}
	error "(non-existent ldp-agent for peer [$dst id] on node [$self id]) \
--- in Node::get-ldp-agent{}"
}

# distribute labels based on routing protocol
Node instproc ldp-trigger-by-routing-table {} {
        if { [[$self set classifier_] cmd control-driven?] != 1 } {
		return
        }
        set ns [Simulator instance]
	for {set i 0} {$i < [$ns array size Node_]} {incr i} {
		# select a node
		set host [$ns get-node-by-id $i]
		if { [$self is-egress-lsr [$host id]] == 1 } {
			# distribute label
			$self ldp-trigger-by-control [$host id] *
		}
        }
}

Node instproc ldp-trigger-by-control {fec pathvec} {
	$self instvar linked_ldpagents_
	lappend pathvec [$self id]
	set inlabel [$self get-incoming-label $fec -1]
	set nexthop [$self get-nexthop $fec]
	for {set i 0} {$i < [llength $linked_ldpagents_]} {incr i 1} {
		set ldpagent [lindex $linked_ldpagents_ $i]
		if { [$ldpagent peer-ldpnode] != $nexthop } {
			if { $inlabel == -1 } {
				if { [$self is-egress-lsr $fec] == 1 } {
					# This is egress LSR
					set inlabel 0
				} else {
					set inlabel [$self new-incoming-label]
					$self in-label-install $fec -1 -1 $inlabel
				}
			}
			$ldpagent new-msgid
			$ldpagent send-mapping-msg $fec $inlabel $pathvec -1
		}
	}   
}

Node instproc ldp-trigger-by-data {reqmsgid src fec pathvec} {
	$self instvar linked_ldpagents_
	if { [$self is-egress-lsr $fec] == 1 } {
		# This is a egress node
		return
	}
	set outlabel [$self get-outgoing-label $fec -1]
	if { $outlabel > -1  } {
		# reroute check !! to establish altanative path
		set outiface [$self get-outgoing-iface $fec -1]
		if { [$self get-link-status $outiface] == "up" } {
			# LSP was already setup
			return
		}
	}
	lappend pathvec [$self id]      
	set nexthop [$self get-nexthop $fec]
	for {set i 0} {$i < [llength $linked_ldpagents_]} {incr i 1} {
		set ldpagent [lindex $linked_ldpagents_ $i]
		if { [$ldpagent peer-ldpnode] != $nexthop } {
			continue
		}
		if {$reqmsgid > -1} {
			# on-demand mode
			set working [$ldpagent msgtbl-get-msgid $fec -1 $src]
			if { $working < 0 } {
				# have to make new request-msg
				set newmsgid [$ldpagent new-msgid]
				$ldpagent msgtbl-install $newmsgid $fec -1 \
						$src $reqmsgid
				$ldpagent send-request-msg $fec $pathvec
			} else {
				# $self is already tring to setup a LSP
				# So, need not to send a request-msg
			}
		} else {
			# not on-demand mode
			if {$fec == $nexthop} {
				# This is a penultimate hop
				set outlabel 0
			} else {
				set outlabel [$self new-outgoing-label]
			}
			$self out-label-install $fec -1 $nexthop $outlabel
			$ldpagent new-msgid
			$ldpagent send-mapping-msg $fec $outlabel $pathvec -1
		}
		return
	}
}

Node instproc make-explicit-route {fec er lspid rc} {
	$self ldp-trigger-by-explicit-route -1 [$self id] $fec "*" $er $lspid $rc
}

Node instproc ldp-trigger-by-explicit-route {reqmsgid src fec pathvec \
		er lspid rc} {
	$self instvar linked_ldpagents_ classifier_
	set outlabel [$self get-outgoing-label $fec $lspid]
	if { $outlabel > -1 } {
		# LSP was already setup
		return
	}
	if { [$self id] != $src && [$self id] == $fec } {
		# This is a egress node
		set ldpagent [$self get-ldp-agent $src]
		$ldpagent new-msgid
		$ldpagent send-cr-mapping-msg $fec 0 $lspid $reqmsgid
		return
	}
	lappend pathvec [$self id]
	set er [split $er "_"]
	set erlen [llength $er]
	for {set i 0} {$i <= $erlen} {incr i 1} {
		if { $i != $erlen } {
			set erhop [lindex $er $i]
		} else {
			set erhop $fec
		}
		set stackERhop -1
		if { $erhop >= [Classifier/Addr/MPLS minimum-lspid] } {
			# This is the ER-Hop and LSPid (ER-LSP)
			set lspidFEC [$self get-fec-for-lspid $erhop]
			set inlabel  [$self get-incoming-label -1 $erhop]
			set outlabel [$self get-outgoing-label -1 $erhop]
			if { $lspidFEC == $fec } {
				# splice two LSPs
				if { $outlabel <= -1 } {
					continue
				}
				if { $inlabel < 0 } {
					set inlabel [$self new-incoming-label]
					$self in-label-install -1 $erhop \
						$src $inlabel
				}
				set ldpagent [$self get-ldp-agent $src]
				$ldpagent new-msgid
				$ldpagent send-cr-mapping-msg $fec $inlabel \
						$lspid $reqmsgid
				return
			}
			# $lspidFEC != $fec
			# do stack operation
			set existExplicitPeer [$self exist-ldp-agent $lspidFEC]
			if { $outlabel > -1 && $existExplicitPeer == "1" } {
				set stackERhop $erhop 
				set erhop $lspidFEC
			} elseif { $outlabel > -1 && $existExplicitPeer == "0" } {
				set nexthop [$self get-outgoing-iface -1 \
						$erhop]
				set iiface  [$self get-incoming-iface -1 \
						$erhop]
				set ldpagent [$self get-ldp-agent $nexthop]
				set working [$ldpagent msgtbl-get-msgid $fec \
						$lspid $src]
				if { $working < 0 } {
					# have to make new request-msg
					set newmsgid [$ldpagent new-msgid]
					$ldpagent msgtbl-install $newmsgid \
						$fec $lspid $src $reqmsgid
					# determine whether labelpass or 
					# labelstack
					if {($iiface == $src) && \
							($inlabel > -1) } {
						$ldpagent msgtbl-set-labelpass $newmsgid
					} else {
						$ldpagent msgtbl-set-labelstack $newmsgid $erhop
					}
					$ldpagent send-cr-request-msg $fec \
							$pathvec $er $lspid $rc
				}
				return
			} else {
				continue
			}
		}
		if { [lsearch $pathvec $erhop] < 0 } {
			set nexthop [$self get-nexthop $erhop]
			if { [$self is-egress-lsr $nexthop] == 1 } {
				# not exist MPLS-node to process a required 
				# er-LSP
				set ldpagent [$self get-ldp-agent $src]
				if { $erhop == $fec } {
					# This is a final node
					$ldpagent new-msgid
					$ldpagent send-cr-mapping-msg $fec 0 \
							$lspid $reqmsgid
				} else {
					# be a wrong er list
					$ldpagent new-msgid
					$ldpagent send-notification-msg \
							"NoRoute" $lspid
				}
			} else {
				set ldpagent [$self get-ldp-agent $nexthop]
				set working [$ldpagent msgtbl-get-msgid $fec \
						$lspid $src]
				if { $working < 0 } {
					# have to make new request-msg
					set newmsgid [$ldpagent new-msgid]
					set id [$ldpagent msgtbl-install \
							$newmsgid $fec \
							$lspid $src $reqmsgid]
					## for label stack
					if { $stackERhop > -1 } {
						$ldpagent msgtbl-set-labelstack $newmsgid $stackERhop
					}
					$ldpagent send-cr-request-msg $fec $pathvec $er $lspid $rc
				}
			} 
			return
		}
	}
	set ldpagent [$self get-ldp-agent $src]
	$ldpagent new-msgid
	$ldpagent send-notification-msg "NoRoute" $lspid
}

Node instproc ldp-trigger-by-withdraw {fec lspid} {
	$self instvar linked_ldpagents_ 

	set inlabel  [$self get-incoming-label $fec $lspid]
	set iniface  [$self get-incoming-iface $fec $lspid]

	$self in-label-clear $fec $lspid
	
	if {$iniface > -1} {
		for {set i 0} {$i < [llength $linked_ldpagents_]} {incr i 1} {
			set ldpagent [lindex $linked_ldpagents_ $i]
			if { [$ldpagent peer-ldpnode] == $iniface } {
				$ldpagent new-msgid
				$ldpagent send-withdraw-msg $fec $lspid
			}
		}
	} else {
		# several upstream nodes may share a label.
		# so inform all upstream nodes to withdraw the label
		set nexthop [$self get-nexthop $fec]
		for {set i 0} {$i < [llength $linked_ldpagents_]} {incr i 1} {
			set ldpagent [lindex $linked_ldpagents_ $i]
			if { [$ldpagent peer-ldpnode] == $nexthop } {
				continue
			}
			$ldpagent new-msgid
			$ldpagent send-withdraw-msg $fec $lspid
		}
	}   
}

Node instproc ldp-trigger-by-release {fec lspid} {
	$self instvar linked_ldpagents_ 
	set outlabel  [$self get-outgoing-label $fec $lspid]
	if {$outlabel < 0} {
		return
	}
	set nexthop [$self get-outgoing-iface $fec $lspid]
	$self out-label-clear $fec $lspid 
	for {set i 0} {$i < [llength $linked_ldpagents_]} {incr i 1} {
		set ldpagent [lindex $linked_ldpagents_ $i]
		if { [$ldpagent peer-ldpnode] == $nexthop } {
			$ldpagent new-msgid
			$ldpagent send-release-msg $fec $lspid
		}
	}   
}

# Debugging

Node instproc trace-mpls {} {
        $self instvar classifier_ 
        $classifier_ set trace_mpls_ 1
}

Node instproc trace-ldp {} {
        $self instvar linked_ldpagents_
        for {set i 0} {$i < [llength $linked_ldpagents_]} {incr i 1} {
		set ldpagent [lindex $linked_ldpagents_ $i]
		$ldpagent set trace_ldp_ 1
	}
}

Node instproc trace-msgtbl {} {
	$self instvar linked_ldpagents_
	puts "[$self id] : message-table"
	for {set i 0} {$i < [llength $linked_ldpagents_]} {incr i 1} {
		set ldpagent [lindex $linked_ldpagents_ $i]
		$ldpagent msgtbl-dump
	}
}

Node instproc pft-dump {} {
	$self instvar classifier_ 
        
        # dump the records within Partial Forwarding Table(PFT)
        set nodeid [$self id]
        $classifier_ PFTdump $nodeid
}

Node instproc erb-dump {} {
	$self instvar classifier_ 
        
        # dump the records within Explicit Route information Base(ERB)
        set nodeid [$self id]
        $classifier_ ERBdump $nodeid
}

Node instproc lib-dump {} {
	$self instvar classifier_ 
        
        # dump the records within Label Information Base(LIB)
        set nodeid [$self id]
        $classifier_ LIBdump $nodeid
}
