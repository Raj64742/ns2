# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
# Time-stamp: <2000-08-29 11:14:34 haoboy>
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
#  $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/mpls/ns-mpls-ldpagent.tcl,v 1.1 2000/08/29 19:28:03 haoboy Exp $

###########################################################################
# Copyright (c) 2000 by Gaeil Ahn                                	  #
# Everyone is permitted to copy and distribute this software.		  #
# Please send mail to fog1@ce.cnu.ac.kr when you modify or distribute     #
# this sources.								  #
###########################################################################

#############################################################
#                                                           #
#     File: File for LDP protocol                           #
#     Author: Gaeil Ahn (fog1@ce.cnu.ac.kr), Jan. 2000      #
#                                                           #
#############################################################

Agent/LDP instproc init {args} {
	$self set peer_node_ 0
	eval $self next $args
}

Agent/LDP instproc new-msgid {} {
	$self instvar new_msgid_
	return [incr new_msgid_]
}

Agent/LDP instproc peer-ldpnode {} {
	$self instvar peer_node_
	return [$self set peer_node_]
}

Agent/LDP instproc send-notification-msg {status lspid} {
	$self set fid_ 101
	$self cmd notification-msg $status $lspid
}

Agent/LDP instproc send-request-msg {fec pathvec} {
	$self set fid_ 102
	$self request-msg $fec $pathvec
}

Agent/LDP instproc send-mapping-msg {fec label pathvec reqmsgid} {
	$self set fid_ 103
	$self cmd mapping-msg $fec $label $pathvec $reqmsgid
}

Agent/LDP instproc send-withdraw-msg {fec lspid} {
	$self set fid_ 104
	$self withdraw-msg $fec $lspid
}

Agent/LDP instproc send-release-msg {fec lspid} {
	$self set fid_ 105
	$self release-msg $fec $lspid
}

Agent/LDP instproc send-cr-request-msg {fec pathvec er lspid rc} {
	$self set fid_ 102
	$self cr-request-msg $fec $pathvec $er $lspid $rc
}

Agent/LDP instproc send-cr-mapping-msg {fec inlabel lspid prvmsgid} {
	$self set fid_ 103
	$self cr-mapping-msg $fec $inlabel $lspid $prvmsgid
}

Agent/LDP instproc get-request-msg {msgid src fec pathvec} {
	$self instvar node_

	set pathvec [split $pathvec "_"]
	if {[lsearch $pathvec [$node_ id]] > -1} {
		# loop detected
		set ldpagent [$node_ get-ldp-agent $src]
		$ldpagent new-msgid
		$ldpagent send-notification-msg "LoopDetected" -1           
		
		return
	}
	
	set nexthop [$node_ get-nexthop $fec]
	if {$src == $nexthop} {
		#  message from downstream node
		$self request-msg-from-downstream $msgid $src $fec $pathvec
	} else {
		#  message from upstream node
		$self request-msg-from-upstream $msgid $src $fec $pathvec
	}
}

Agent/LDP instproc request-msg-from-downstream {msgid src fec pathvec} {
	$self instvar node_

	#  1. allocate label (upstream label allocation)
	set outlabel [$node_ get-outgoing-label $fec -1]
	if { $outlabel < 0 } {
		if { $fec == $src } {
			# This is a penultimate hop
			set outlabel 0
		} else {
			set outlabel [$node_ new-outgoing-label]
		}     
		$node_ out-label-install $fec -1 $src $outlabel             
	} else {
		set outIface [$node_ get-outgoing-iface $fec -1]
		if { $src != $outIface} {
			# wrong LIB information, let's fix it up
			$node_ out-label-install $fec -1 $src $outlabel
		} 
	}
	
	#  2. reply a allocated label to $src
	set ldpagent [$node_ get-ldp-agent $src]
	$ldpagent new-msgid
	$ldpagent send-mapping-msg $fec $outlabel "*" $msgid

	#  3. trigger label distribution (control-driven-trigger)
	#     not support on-demand mode in control-driven-trigger
	$node_ ldp-trigger-by-control $fec $pathvec
}

Agent/LDP instproc request-msg-from-upstream {msgid src fec pathvec} {
	$self instvar node_

	#  ldp agent for src
	set ldpagent [$node_ get-ldp-agent $src]

	# This is a egress LSR
	if { [$node_ is-egress-lsr $fec] == 1 } {
                $ldpagent new-msgid
                $ldpagent send-mapping-msg $fec 0 "*" $msgid
                return
	}
	
	set inlabel  [$node_ get-incoming-label $fec -1]
	set outlabel [$node_ get-outgoing-label $fec -1]
	#
	# ordered-control mode
	#
	if { [Classifier/Addr/MPLS ordered-control?] == 1 } {
		if { $outlabel > -1 } {
			if { $inlabel < 0 } {
				set inlabel [$node_ new-incoming-label]
			}
			$ldpagent new-msgid
			$ldpagent send-mapping-msg $fec inlabel "*" $msgid
		} else {
			$node_ ldp-trigger-by-data $msgid $src $fec $pathvec
		}
		return
	}
	#
	#  independent-control mode
	#
	#  1. allocate label (upstream label allocation)
	if { $inlabel < 0 } {
		set inlabel [$node_ new-incoming-label]
		$node_ in-label-install $fec -1 $src $inlabel
	} else {
		set inIface [$node_ get-incoming-iface $fec -1]
		if { $src != $inIface} {
			set classifier [$node_ set classifier_]
			set dontcare [$classifier set dont_care_]
			$node_ in-label-install $fec -1 -1 $dontcare
		} 
	}
	#  2. reply a allocated label to $src
	$ldpagent new-msgid
	$ldpagent send-mapping-msg $fec $inlabel "*" $msgid
	#  3. trigger label distribution (data-driven-trigger)
	#     support on-demand mode in data-driven-trigger
	$node_ ldp-trigger-by-data $msgid $src $fec $pathvec
}

Agent/LDP instproc get-cr-request-msg {msgid src fec pathvec er lspid rc} {
	$self instvar node_

	#
	#  CR-LDP:  used to setup explicit-LSP
	#
	set pathvec [split $pathvec "_"]
	if {[lsearch $pathvec [$node_ id]] > -1} {
		# loop detected
		set ldpagent [$node_ get-ldp-agent $src]
		$ldpagent new-msgid
		$ldpagent send-notification-msg "NoRoute" $lspid
		return
	}
	
	# reserve a required resources using $rc
	# if a reservation fails, send notificaiton-msg

	#  ldp agent for src
	set ldpagent [$node_ get-ldp-agent $src]

	#  if a outgoing label for the fec exists ingnore this request-msg and
	#  send cr-mapping-msg.
	set inlabel [$node_ get-incoming-label $fec $lspid]
	set outlabel [$node_ get-outgoing-label $fec $lspid]
	
	if { $outlabel > -1 } {
                if { $inlabel < 0 } {
			set inlabel [$node_ new-incoming-label]
			$node_ in-label-install $fec $lspid $src $inlabel
                }
                $ldpagent new-msgid
                $ldpagent send-cr-mapping-msg $fec $inlabel $lspid $msgid
                return
	}

	$node_ ldp-trigger-by-explicit-route $msgid $src $fec $pathvec $er $lspid $rc
}

Agent/LDP instproc get-cr-mapping-msg {msgid src fec label lspid reqmsgid} {
	$self instvar node_ trace_ldp_

	# select a upstream-node to receive a mapping-msg in msgtbl
	# and clear the msgtbl entry

	set prvsrc   [$self msgtbl-get-src       $reqmsgid]
	set prvmsgid [$self msgtbl-get-reqmsgid  $reqmsgid]
	set labelop  [$self msgtbl-get-labelop   $reqmsgid]
	if {$labelop == 2} {
		set tunnelid [$self msgtbl-get-erlspid   $reqmsgid]
	} else {
		set tunnelid -1
	}
	
	$self msgtbl-clear $reqmsgid

	if { $trace_ldp_ } {
		puts "$src -> [$node_ id] : prvsrc($prvsrc)"
	}
	
	# allocate outgoing-label
	if { $labelop == 2 } {
		# stack operation
		$node_ out-label-install $fec $lspid $src $label
		$node_ erlsp-stacking $lspid $tunnelid
	} elseif {$labelop == 1} {
		# only pass label
		set ldpagent [$node_ get-ldp-agent $prvsrc]
		$ldpagent new-msgid
		$ldpagent send-cr-mapping-msg $fec $label $lspid $prvmsgid
		return
	} else {
		# use the label
		$node_ out-label-install $fec $lspid  $src $label
	}
	
	if {$prvsrc == [$node_ id]} {
		return
	}

	# allocate incoming-label            
	set inlabel [$node_ new-incoming-label]
	$node_ in-label-install $fec $lspid $prvsrc $inlabel

	# find a ldpagent for a upstream-node, continually send a 
	# mapping-msg to the node
	set ldpagent [$node_ get-ldp-agent $prvsrc]
	$ldpagent new-msgid
	$ldpagent send-cr-mapping-msg $fec $inlabel $lspid $prvmsgid
}

# msgid    : the msgid of a mapping message sent by dstination
# reqmsgid : the msgid of a request message sent by me before
Agent/LDP instproc get-mapping-msg {msgid src fec label pathvec reqmsgid} {
	$self instvar node_ trace_ldp_

	if { $trace_ldp_ } {
		puts "[[Simulator instance] now]: <mapping-msg> $src ->\
[$node_ id] : fec($fec), label($label) [$node_ get-nexthop $fec]"
	}
	
	set pathvec [split $pathvec "_"]
	if {[lsearch $pathvec [$node_ id]] > -1} {
		# loop detected
		set ldpagent [$node_ get-ldp-agent $src]
		$ldpagent new-msgid
		$ldpagent send-notification-msg "LoopDetected" -1           
		return
	}

	set nexthop [$node_ get-nexthop $fec]
	if {$src == $nexthop} {
		#  message from downstream node
		$self mapping-msg-from-downstream $msgid $src $fec $label \
				$pathvec $reqmsgid
	} else {
		#  message from upstream node
		$self mapping-msg-from-upstream $msgid $src $fec $label \
				$pathvec $reqmsgid
	}
}

Agent/LDP instproc mapping-msg-from-downstream {msgid src fec label \
		pathvec reqmsgid} {
	$self instvar node_

	#  allocate label (upstream label allocation)
	$node_ out-label-install $fec -1 $src $label
	if { $reqmsgid > -1 } {
		# downstream-on-demand mode
		
		# 1. find a upstream-node who sent a request-msg in msgtbl
		#    & delete entry in msgtbl
		set prvsrc   [$self msgtbl-get-src      $reqmsgid]
		set prvmsgid [$self msgtbl-get-reqmsgid $reqmsgid]
		$self msgtbl-clear $reqmsgid
		if { $prvsrc == [$node_ id] || $prvsrc < 0} {
			return
		}
		if { [Classifier/Addr/MPLS ordered-control?] == 1 } {
			# ordered-control mode

			# 1. allocate incoming-label
			set inlabel [$node_ new-incoming-label]
			$node_ in-label-install $fec -1 $prvsrc $inlabel
			# 2. find a ldpagent for a upstream-node,
			#    send a mapping-msg forward a upstream-node
			set ldpagent [$node_ get-ldp-agent $prvsrc]
			$ldpagent new-msgid
			$ldpagent send-mapping-msg $fec $inlabel -1 $prvmsgid
			return
		}
	} else {
		#  trigger label distribution (control-driven-trigger)
		$node_ ldp-trigger-by-control $fec $pathvec
		return
	}
}

Agent/LDP instproc mapping-msg-from-upstream {msgid src fec label pathvec \
		reqmsgid} {
	$self instvar node_

	# check whether or not myself is a next hop of src for a fec.
	set nexthop [$node_ lookup-nexthop $src $fec]
	if { $nexthop != [$node_ id] } {
		return
	}

	#  1. allocate label (upstream label allocation)
	set inlabel [$node_ get-incoming-label $fec -1]
	if { $inlabel == -1 } {
		if { [$node_ is-egress-lsr $fec] == 1 } {
			#  message from a penultimate hop
			#  so, send a null label to a penultimate hop
			if { $label != 0 } {
				set ldpagent [$node_ get-ldp-agent $src]
				$ldpagent new-msgid
				$ldpagent send-mapping-msg $fec 0 "*" $msgid
			}
		} else {
			$node_ in-label-install $fec -1 $src $label
		}
	} else {
		set ldpagent [$node_ get-ldp-agent $src]
		if { $reqmsgid < 0 } {
			$ldpagent new-msgid
			$ldpagent send-mapping-msg $fec $inlabel "*" $msgid
		}   
	}    
	#  2. trigger label distribution (data-driven-trigger)
	#     support on-demand mode only in data-driven-trigger
	if { $reqmsgid < 0 } {
		$node_ ldp-trigger-by-data -1 $src $fec $pathvec
	} else {   
		# if ($reqmsgid > -1) then delete a entry in msgid table
		$self msgtbl-clear $reqmsgid
	}    
}

Agent/LDP instproc get-notification-msg {src status lspid} {
	$self instvar node_ trace_ldp_

	# Notification-msg (in case of Loop Detection, No Route)
	if { $trace_ldp_ } {
		puts "Notification($src->[$node_ id]): $status src=$src lspid=$lspid"
	}
	set msgid [$self msgtbl-get-msgid -1 $lspid -1]
	if {$msgid > -1} {
		set prvsrc   [$self msgtbl-get-src      $msgid]
		$self msgtbl-clear $msgid            
		if { $prvsrc < -1 || $prvsrc == [$node_ id] } {
			return
		}
		set ldpagent [$node_ get-ldp-agent $prvsrc]
		$ldpagent new-msgid
		$ldpagent send-notification-msg $status $lspid
	}
}

Agent/LDP instproc get-withdraw-msg {src fec lspid} {
	$self instvar node_

	# withdraw msg : downstream -> upstream
	set outiface  [$node_ get-outgoing-iface $fec $lspid]
	if {$src == $outiface} {
		$node_ out-label-clear $fec $lspid
		set inlabel [$node_ get-incoming-label $fec $lspid]
		if {$inlabel > -1} {
			$node_ ldp-trigger-by-withdraw $fec $lspid
		}
	}
}

Agent/LDP instproc get-release-msg {src fec lspid} {
	$self instvar node_
	
	# release msg : upstream --> downstream
	set iniface  [$node_ get-incoming-iface $fec $lspid]
	set outlabel [$node_ get-outgoing-label $fec $lspid]
	if {$iniface == $src} {
		$node_ in-label-clear $fec $lspid 
		if {$outlabel > -1} {
			$node_ ldp-trigger-by-release $fec $lspid
		}
	} 
}

Agent/LDP instproc trace-ldp-packet {src_addr src_port msgtype msgid fec \
		label pathvec lspid er rc reqmsgid status atime} {
	$self instvar node_
	puts "$atime [$node_ id]: $src_addr ($msgtype $msgid) $fec $label $pathvec  \[$reqmsgid $status\]  \[$lspid $er $rc\]"
}
