#
# tcl/mcast/McastProto.tcl
#
# Copyright (C) 1997 by USC/ISI
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 
# Ported by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
# 
#
Class McastProtocol

McastProtocol instproc init {sim node} {
	$self next
	$self instvar ns_ node_ status_ type_ id_
	set ns_   $sim
	set node_ $node
	set status_ "down"
	set type_   [$self info class]
	set id_ [$node id]

        [$node_ getArbiter] addproto $self
	$ns_ maybeEnableTraceAll $self $node_
}

McastProtocol instproc getType {} { $self set type_ }

McastProtocol instproc start {}		{ $self set status_ "up"   }
McastProtocol instproc stop {}		{ $self set status_"down"  }
McastProtocol instproc getStatus {}	{ $self set status_	   }

McastProtocol instproc upcall {code args} {
	# currently expects to handle cache-miss and wrong-iif
	eval $self handle-$code $args
}
 
McastProtocol instproc handle-wrong-iif args {
	# NOTHING
	return 1
}

McastProtocol instproc annotate args {
	$self instvar dynT_ node_ ns_
	set s "[$ns_ now] [$node_ id] $args" ;#nam wants uinique first arg???
	if [info exists dynT_] {
		foreach tr $dynT_ {
			$tr annotate $s
		}
	}
}

McastProtocol instproc join-group arg	{ 
	$self annotate $proc $arg 
}
McastProtocol instproc leave-group arg	{ 
	$self annotate $proc $arg
}

McastProtocol instproc trace { f src {op ""} } {
        $self instvar ns_ dynT_
	if {$op == "nam" && [info exists dynT_] > 0} {
		foreach tr $dynT_ {
			$tr namattach $f
		}
	} else {
		lappend dynT_ [$ns_ create-trace Generic $f $src $src $op]
	}
}

McastProtocol instproc notify changes {
	# NOTHING
}
McastProtocol instproc dump-routes args {
	# NOTHING
}

# Find out what interface packets sent from $node will arrive at
# this node. $node need not be a neighbor. $node can be a node object
# or node id.
McastProtocol instproc from-node-iface { node } {
	$self instvar ns_ node_
	catch {
		set node [$ns_ get-node-by-id $node]
	}
	set rpfnbr [$node_ rpf-nbr $node]
	if {![catch { set rpflink [$ns_ link $rpfnbr $node_]}]} {
		return [$rpflink if-label?]
	}
	return "?" ;#unknown iface
}

McastProtocol instproc rpf-nbr src {
	$self instvar node_
	$node_ rpf-nbr $src
}


McastProtocol instproc iface2link ifid {
	$self instvar node_
	$node_ if2link $ifid
}

McastProtocol instproc link2iif link {
	$self instvar node_
	if { [$link dst] != $node_ } {
		# oops...naughty, naughty
	}
	$link if-label?
}

McastProtocol instproc link2oif link {
	$self instvar node_
	if { [$link src] != $node_ } {
		# ooops, naughty, naughty
	}
	$node_ link2oif $link
}


###################################################
Class mrtObject

#XXX well-known groups (WKG) with local multicast/broadcast
mrtObject set mask-wkgroups	0xfff0
mrtObject set wkgroups(Allocd)	[mrtObject set mask-wkgroups]

mrtObject proc registerWellKnownGroups name {
	set newGroup [mrtObject set wkgroups(Allocd)]
	mrtObject set wkgroups(Allocd) [expr $newGroup + 1]
	mrtObject set wkgroups($name)  $newGroup
}

mrtObject proc getWellKnownGroup name {
	assert "\"$name\" != \"Allocd\""
	mrtObject set wkgroups($name)
}

mrtObject registerWellKnownGroups ALL_ROUTERS
mrtObject registerWellKnownGroups ALL_PIM_ROUTERS

mrtObject proc expandaddr {} {
	# extend the space to 32 bits
	mrtObject set mask-wkgroups	0x7ffffff0

	foreach {name group} [mrtObject array get wkgroups] {
		mrtObject set wkgroups($name) [expr $group | 0x7fffffff]
	}
}

# initialize with a list of the mcast protocols
mrtObject instproc init { node protos } {
        $self next
	$self set node_	     $node
	$self set protocols_ $protos
}

mrtObject instproc addproto { proto } {
        $self instvar protocols_
        lappend protocols_ $proto
}

mrtObject instproc getType { protocolType } {
        $self instvar protocols_
        foreach proto $protocols_ {
                if { [$proto getType] == $protocolType } {
                        return $proto
                }
        }
        return ""
}

mrtObject instproc all-mprotos {op args} {
	$self instvar protocols_
	foreach proto $protocols_ {
		eval $proto $op $args
	}
}

mrtObject instproc start {}	{ $self all-mprotos start	}
mrtObject instproc stop {}	{ $self all-mprotoc stop	}
mrtObject instproc notify changes { $self all-mprotos notify $changes }
mrtObject instproc dump-mroutes args {
	eval all-mprotos dump-routes $args
}

# similar to membership indication by igmp.. 
mrtObject instproc join-group args {	;# args = < G [, S] >
	eval $self all-mprotos join-group $args
}

mrtObject instproc leave-group args {	;# args = < G [, S] >
	eval $self all-mprotos leave-group $args
}

mrtObject instproc upcall { code source group iface } {
  # check if the group is local multicast to well-known group
	set wkgroup [expr [$class set mask-wkgroups]]
	if { [expr ( $group & $wkgroup ) == $wkgroup] } {
                $self instvar node_
		$node_ add-mfc $source $group -1 {}
        } else {
		$self all-mprotos upcall $code $source $group $iface
	}
}

mrtObject instproc drop { replicator src dst {iface -1} } {
        $self all-mprotos drop $replicator $src $dst $iface
}
