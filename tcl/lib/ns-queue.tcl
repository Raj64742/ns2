#
# This file contains auxillary support for CBQ and CBQ links, and
# some embryonic stuff for Queue Monitors. -KF
#

#
# CBQ
#

#
# set up the CBQ/Queue object, link instvars, and classifier
# once more complex classifiers are introduced, creating the classifier
# will probably be done elsewhere
#
Class CBQLink -superclass SimpleLink
CBQLink instproc init { src dst bw delay q {lltype "DelayLink"} } {
        $self next $src $dst $bw $delay $q $lltype
        $self instvar head_ queue_ link_ classifier_
	$queue_ link $link_
        set classifier_ [new Classifier/Hash/Fid 32]
	set head_ $classifier_
	#
	# the following is merely to inject 'algorithm_' in the
	# $queue_ class' name space.  It is not used in ns-2, but
	# is needed here for the compat code to recognize that
	# 'set algorithm_ foo' commands should work
	#
	$queue_ set algorithm_ [Queue/CBQ set algorithm_]

}

#
#
# for flow-id based classification, bind c
# CBQClass to a given flow id # (or range)
#
# OTcl usage:
# 	bind $cbqclass id
#    		or
# 	bind $cbqclass idstart idend
#
# these use flow id's as id's
#
CBQLink instproc bind args {
	# this is to perform '$cbqlink bind $cbqclass id'
	# and '$cbqlink insert $cbqclass bind $cbqclass idstart idend'

	$self instvar classifier_
	set nargs [llength $args]
	set cbqcl [lindex $args 0]
	set a [lindex $args 1]
	if { $nargs == 3 } {
		set b [lindex $args 2]
	} else {
		set b $a
	}
	# bind the class to the flow id's [a..b]
	while { $a <= $b } {
		$classifier_ set-hash fid $cbqcl $a
		incr a
	}
}

#
# insert the class into the link
# each class will have an associated queue
# we must create a set of queue monitors around these queues which
# cbq uses to monitor demand
#
# general idea:
#  pkt--> Classifier --> CBQClass --> snoopin --> qdisc --> snoopout --> CBQ
#
CBQLink instproc insert cbqcl {
	# queue_ refers to the cbq object
	$self instvar queue_ snoopDrop_
	set qdisc [$cbqcl qdisc]

	# qdisc can be null for internal classes
	if { $qdisc != "" } {
		# create in, out, and drop snoop queues
		# and attach them to the same monitor
		# this is used by CBQ to assess demand
		# (we don't need bytes/pkt integrator or stats here)
		set qmon [new QueueMonitor]
		set in [new SnoopQueue/In]
		set out [new SnoopQueue/Out]
		set drop [new SnoopQueue/Drop]
		$in set-monitor $qmon
		$out set-monitor $qmon
		$drop set-monitor $qmon

		# output of cbqclass -> snoopy inq
		$in target $qdisc
		$cbqcl target $in

		# drop from qdisc -> snoopy dropq
		# snoopy dropq's target is overall cbq drop target
		$qdisc drop-target $drop
		if [info exists snoopDrop_] {
			$drop target $snoopDrop_
		} else {
			$drop target [ns set nullAgent_]
		}

		# output of queue -> snoopy outq
		# output of snoopy outq is cbq
		$qdisc target $out
		$out target $queue_
		# tell this class about its new queue monitor
		$cbqcl qmon $qmon
	}

	# tell cbq about this class
	# (this also tells the cbqclass about the cbq)
	$queue_ insert-class $cbqcl
}
#
# procedures on a cbq class
#

CBQClass instproc setparams { parent okborrow allot maxidle prio level xdelay } {

puts "setparam: okborrow $okborrow"
        $self allot $allot
	$self parent $parent

	$self set okborrow_ $okborrow
        $self set maxidle_ $maxidle
        $self set priority_ $prio
        $self set level_ $level
        $self set extradelay_ $xdelay

        return $self
}

#
# insert a queue into a CBQ class:
#	arrange for the queue to initally be blocked
#	and for it to block when resumed
#	(provides flow control on the queue)

CBQClass instproc install-queue q {
	$q set blocked_ true
	$q set unblock_on_resume_ false
	$self qdisc $q
}
