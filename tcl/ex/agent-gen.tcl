# Created May 98 by Ahmed Helmy; updated June 98
# agent generator class
Class AgentGen

# TODO: 
# - add topological semantics (e.g. stubs) for the
# randomization and <relative> placement of srcs and rcvrs
# (need api that gets the node ranges from the topology generator)
# 

proc agent-usage { } {
	puts stderr {usage: agents [options]

where options are given as: -key value
example options:
-outfile f -transport TCP/Reno -num 20 -src FTP -sink TCPSink 
-start 1-3 -stop 6-8 

"agents -h" for help
}
	return
}

proc detailed-usage { } {
puts {usage: agents [-<key 1> <value 1> -<key n> <value n>]

example:
agents -outfile f -transport TCP -num 20 -src FTP -sink TCPSink

Keys:
-outfile: the filename to which the generated script will be
	written.

-transport: [now supports only TCP and SRM and their variants]
 values: 
  tcp types:
	 TCP [TCP Tahoe], TCP/Reno, TCP/NewReno, TCP/Sack1, 
	 TCP/Vegas, TCP/Fack
  srm types:
	SRM, SRM/Deterministic , SRM/Probabilistic, SRM/Adaptive
	details:
	SRM: C1 = C2 = 2 (request param), D1 = D2 = 1 (repair param)
	Deterministic: C2 = 0, D2 = 0
	Probabilistic: C1 = D1 = 0


-src: FTP [default with TCPs], Telnet, CBR, CBR/UDP.
	CBR is a constant bit rate source,
	CBR/UDP same as CBR but with ability to use traffic 
		models [such as pareto and exponential, described next]

-traffic: [the traffic model, may be used with CBR/UDP]
	Expoo: Exponential on/off model
	Pareto: Pareto on/off model

-sink: [now used for sinks of tcp connections]
 values:
 TCPSink,TCPSink/DelAck,TCPSink/Sack1,TCPSink/Sack1/DelAck,FullTcp 
	details:
	one way rxvg agents: TCPSink (1 ack per packet) , 
	TCPSink/DelAck (w/ delay per ack), TCPSink/Sack1 (selective ack
	sink rcf2018), TCPSink/Sack1/DelAck (sack1 with DelAck)
	2 way "FullTcp": Reno sender
-num: number of tcp connections or total srm agents.
	Given as either absolute value indicating number of
	nodes, or as percentage of the total number of nodes in
	the topology (e.g. 60%).
	[for srm the default is all nodes are agents]

-srcnum: the number of srm agents that are also sources
	[defaults to 10% of total nodes]

-start: starting time of the tcp connections, or srm sources. May
	be given as a range x1-x2, in which different random
	starting times from the range will be assigned to
	different agents, or may be given as value x at which 
	all agents start (default is 0)

-stop: (format similar to start) stop the sources

-join: (format similar to start) when srm agents join the group

-leave: (format similar to start) when srm agents leave
	[not support by srm yet!]

-totalnodes: the number of nodes to be used by the agent
		generator as the total # nodes in the topology.
		[if left out, the agent generator asks the
		topology generator for this info.]
  }
}

proc agents { args } {
	set len [llength $args]

	if { $len } {
	    set key [lindex $args 0]
            if {$key == "-?" || $key == "--help" || $key == "-help" \
			|| $key == "-h" } {
			detailed-usage
			return
                }
	}

        if { !$len || [expr $len % 2] } {
                # if number is odd => error !
                puts "fewer number of arguments than needed in \"$args\""
		agent-usage
		return
        }

	if { [catch {set ag [AgentGen info instances]}] } {
		puts "can't create AgentGen ..!!"
		agent-usage
		return
	}
	if { $ag == "" } {
		set ag [new AgentGen]
	}
	$ag create $args
}

AgentGen instproc init { } {
	$self next
}

AgentGen instproc default_options { } {

	$self instvar opt_info

	set opt_info {
		# init file to -1, must be supplied by input
		outfile -1

		transport -1

		src -1

		sink -1

		num -1

		# may add location same_stub/other_stub later

		# start range either a number x or range x1-x2
		start 0

		stop 0

		# srm srcnum
		srcnum 10%
	
		traffic -1

		join  0
		leave 0

		totalnodes 0
	}
	$self parse_opts
}
	
AgentGen instproc parse_opts { } {
	$self instvar opts opt_info

	while { $opt_info != ""} {
		# parse line by line
                if {![regexp "^\[^\n\]*\n" $opt_info line]} {
                        break  
                }
		# remove the parsed line
                regsub "^\[^\n\]*\n" $opt_info {} opt_info
		# remove leading spaces and tabs using trim
                set line [string trim $line]
		# skip comment lines beginning with #
                if {[regexp "^\[ \t\]*#" $line]} {
                        continue
                }
		# skip empty lines
                if {$line == ""} {
                        continue
                } elseif {[regexp {^([^ ]+)[ ]+([^ ]+)$} $line dummy key value]} {
                        set opts($key) $value
                } 
	}
}

AgentGen instproc parse_input { args } {
	# remove the list brackets from the args list
        set args [lindex $args 0]
        set len [llength $args]

	$self instvar opts	

	for { set i 0 } { $i < $len } { incr i } {
		set key [lindex $args $i]
		regsub {^-} $key {} key
                if {![info exists opts($key)]} {
			puts stderr "unrecognized option $key"
			agent-usage
			return -1
		}
		incr i
		# puts "changing $key from $opts($key) to [lindex $args $i]"
		set opts($key) [lindex $args $i]
	}
	# puts "end of parsing... "
	return 0
}

AgentGen instproc create { args } {
        # remove the list brackets from the args list
        set args [lindex $args 0]
        set len [llength $args]
        # puts "calling create with args $args, len $len"

	$self default_options

	if { $len } {
		if { [$self parse_input $args] == -1 } {
			return 
		}
	}

	# check that the filename is provided
	$self instvar opts
	if { $opts(outfile) == -1 } {
	 puts {you must provide outfile. Use "agents -h" for help}
		return
	}
	$self save-command $opts(outfile) $args
	$self create-agents
}

# we save the commands to append them to the end of the file
AgentGen instproc save-command { file command } {
	global AllCommandLines
	# XXX clear the commands if this is a new file
	if { ![file exists $file] } {
		set f [open $file w]
		# XXX complete
		# puts $f "header info... "
puts $f "\n proc generate-agents \{ sim nodes \} \{ \n"
puts $f "\t upvar \$nodes n; upvar \$sim ns \n\n"
		flush $f; close $f
		set AllCommandLines ""
	}
	$self instvar commandLine
	set commandLine "agents $command"
	lappend AllCommandLines $commandLine
}

AgentGen instproc create-agents { } {

	$self instvar opts

	# puts "transport $opts(transport)"

	set transport -1
	regexp {^([a-zA-Z]+)} $opts(transport) all transport

	switch $transport {
		"TCP" {
		  if { [$self check-tcp $opts(transport)] == -1 || \
			[$self check-tcp-sink $opts(sink)] == -1 } {
		    puts "invalid or unsupported tcp or sink option"
			return -1
		  }
		  set tcp $opts(transport)
		  set sink $opts(sink)
		  set src $opts(src)
		  if { $src == -1 } {
			puts "you left out the source..!! using FTP"
			set src FTP
		  }
		  set num [$self calc-num $opts(num)]

		# we open the file in append mode since we allow 
		# multiple agents commands in one script...
		  set f [open $opts(outfile) a]

		  set str [$self preamble-tcp]
		  puts $f "$str"

		# may add topological semantics for src,dst
		# placemnt .. later.. for now use all (i.e.
		# randomize over all nodes)
		  set str [$self generate-tcp-agents $tcp $src \
				$sink $num all]
		  puts $f "$str"
		  
		  set start $opts(start)
		  set stop $opts(stop)
		  set str [$self generate-src-start $start $stop $num]
		  puts $f "$str"

		  flush $f
		  close $f
		}

		# can merge the checks in check-$type proc..later
		"SRM" {
		# may want to add a check on multicast and
		# either assign default mrouting, or flag an
		# error if mrouting is not enabled

		  # if left out, use SRM default
		  if { [$self check-srm $opts(transport)] == -1 } {
			puts "invalid or unsupported srm option"
			return -1
		  }
		  set srm $opts(transport)
		  set num [$self calc-num $opts(num)]
		  set srcnum [$self calc-num $opts(srcnum)]

		  set f [open $opts(outfile) a]

		  set str [$self preamble-srm]
		  puts $f "$str"

		  set src $opts(src)
		  set traffic $opts(traffic)

		  set str [$self generate-srm-agents $srm $num \
			$srcnum $src $traffic all]

		  puts $f "$str"

		  # may want to check on these values.. later X
		  set join $opts(join)
		  set leave $opts(leave)
		  set start $opts(start)
		  set stop $opts(stop)

		  set str [$self generate-srm-patterns $join \
			$leave $num $start $stop $srcnum]

		  puts $f "$str"

		  flush $f
		  close $f
		  
		}
		
		"-1" {
		 puts "transport was not specified !!"
		 # should we return for now
		 return -1
		}

		default { 
		  puts "unknown or unsupported transport \"$opts(transport)\""
		  return -1
		}
	}	
}

AgentGen instproc check-tcp { type } {
	set tcps { "" /Reno /NewReno /Sack1 /Vegas /Fack /FullTcp }
	foreach tcp $tcps {
		if { $type == "TCP$tcp" } {
			return 1
		}
	}
	return -1
}

AgentGen instproc check-tcp-sink { sink } {
	set sinks { TCPSink TCPSink/DelAck TCPSink/Sack1 \
	  TCP/Sink/Sack1/DelAck FullTcp }
	foreach snk $sinks {
		if { $snk == $sink } {
			return 1
		}
	}
	return -1
}

AgentGen instproc check-srm { type } {
	set types { "" /Deterministic /Probabilistic /Adaptive }
	foreach srmtype $types {
		if { $type == "SRM$srmtype" } {
			return 1
		}
	}
	return -1
}

# XXX testing hack
# Class TG
# TG proc get-total-nodes { } {
#	return 25
# }

AgentGen instproc getNodes { range } {
	$self instvar opts
	if { $range == "all" } {
	   if { !$opts(totalnodes) } {
		# total nodes was not provided at cmd line so ask
		# topology generator
		if {[catch {set tg [ScenGen getTG]}]} {
		 puts stderr {Can't find topology generator..quiting!
Either run the topology generator first, or provide the total
number of nodes using the "totalnodes" option.
Use "agents -h" for more help.}
			exit
		}
		set lastIndex [$tg getNodes all]
		return $lastIndex
	   }
	   return $opts(totalnodes)
		# return 0-$lastIndex
	}
	# should support stubs and other topological semantics.
	# return $firstIndex-$lastIndex
}

# should there be an SRMGen class (and TCPGen class..etc) !

AgentGen instproc calc-num { num } {
	set totalNodes [$self getNodes all]
	if { $num == -1 } {
		# if num is left out, assign a number
		# equal to the number of nodes
		set number $totalNodes
	} elseif { [regexp {^([0-9]+)(%)$} $num all num per] } {
		# check for percentage
		set number [expr $totalNodes * $num / 100]
	} else { set number $num }
	# puts "num $number"
	return $number
}

# should have SRMGen/TCPGen/'transport'Gen generate-agents...etc.
AgentGen instproc generate-srm-agents { srm num srcnum src \
		traffic node_ranges } {
	# puts "generate srm agents $srm $num $srcnum $src $traffic"
	set totalNodes [$self getNodes all]
	if { $node_ranges == "all" } {
	  # we assume that num is less that totalNodes..X
	  set nodes [$self randomize-agents 0 $totalNodes $num]
	} else {
		# not supported yet... use stubs clustering..etc
	}
	set srcNodes [lrange $nodes 0 [expr $srcnum -1]]
	set rcvrNodes [lrange $nodes $srcnum end]

set a_5 "\n # generate $num $srm agents, $srcnum of which are \n"
set a_4 "# srcs $src/$traffic. Distribute randomly. \n"
	set a_1 "\n set group \[Node allocaddr\] \n"
	# create agents assume 1 srm agent per node

# if we are going to use array names and array size, then
# use unset to remove any left over arrays from previous commands
# check that we don't need those arrays again
# e.g. unset srm, unset srmsrc, unset s, unset src... etc XXX

	

	# may create one loop for both srmsrc and srm .. later X
	set a "foreach i \{$rcvrNodes\} \{ \n"
	set b "\t set srm(\$i) \[new Agent/$srm\] \n"
	# set the group ... and other params.. XXX
	set c "\t \$srm(\$i) set dst_ \$group \n"
	set d "\t \$ns attach-agent \$n(\$i) \$srm(\$i) \n \} \n"

	set str1 "$a_5 $a_4 $a_1 $a $b $c $d"

	set ba "foreach i \{$srcNodes\} \{ \n"
	set bb "\t set srmsrc(\$i) \[new Agent/$srm\] \n"
	# set the group ... and other params.. XXX
	set bc "\t \$srmsrc(\$i) set dst_ \$group \n"
	set bd "\t \$ns attach-agent \$n(\$i) \$srmsrc(\$i) \n"

	# assign the srcs
	set f "\t set s(\$i) \[new Agent/$src\] \n"
	
	# traffic is only used with CBR UDP
	if { $src == "CBR/UDP" } {
	 set g "\t set traffic(\$i) \[new Traffic/$traffic\] \n"
	 # assign the trfc params. later... burst, idle, rate..XXX
	 set h "\t \$s(\$i) attach-traffic \$traffic(\$i) \n"
	} else { set g ""; set h ""; }
	set i "\t \$srmsrc(\$i) traffic-source \$s(\$i) \n \} \n"
	set str2 "$ba $bb $bc $bd $f $g $h $i"

	set str "$str1 $str2"
	# puts "generate srm done"
	return $str
}

AgentGen instproc commandLine { } {
	$self instvar commandLine
	return $commandLine
}

AgentGen instproc preamble-tcp { } {
	set separator {#################}
	set a "\n $separator \n"
set ab "# The command line generating this part of the script is: \n"
	set ac "# [$self commandLine]"
	set ad $a
set ae "\n\n # XXX cuz we use \[array names src\], we unset it first.\n"
	set b "catch \{unset src\} \n"
	return "$a $ab $ac $ad $ae $b"
}

AgentGen instproc preamble-srm { } {
	set separator {#################}
	set a "\n $separator \n"
set ab "# The command line generating this part of the script is: \n"
	set ac "# [$self commandLine]"
	set ad $a
set ae "\n\n # XXX cuz we use array names of srm and srmsrc, we unset.\n"
	set b "catch \{unset srmsrc\} \n"
	set c "catch \{unset srm\} \n"
	return "$a $ab $ac $ad $ae $b $c"
}

# node-ranges to be extended to capture topological semantics
AgentGen instproc generate-tcp-agents { tcp src sink num \
		node_ranges} {
# puts "generate tcp $tcp $src $sink $num $node_ranges"
	set totalNodes [$self getNodes all]
	# assume there's an API to get the total num of nodes in 
	# the topology simulated, also we may need an API to 
	# get ranges of topological significance (like stub
	# ranges..etc), topology type.. so on..
	if { $node_ranges == "all" } {
	  set pairs [$self randomize-agent-pairs 0 $totalNodes \
		0 $totalNodes $num]
	} else {
	 	# may use topology info (e.g. stubs) to get
		# the pairs
	}

set a_5 "\n # generate $num connections of $tcp, using $sink \n"
set a_4 "# sink types, and $src sources. Distribute randomly. \n"
	set a_1 "\n set i 0 \n"
	set a "foreach pair \{$pairs\} \{ \n"
	set b "\t set sorc \[lindex \$pair 0\] \n"
	set c "\t set dst \[lindex \$pair 1\] \n"
	# set the fid as srcdst for now
	set d_1 "\t set fid \$sorc\$dst \n"	
set d "\t set tcp(\$i) \[\$ns create-connection $tcp \$n(\$sorc) $sink \$n(\$dst) \$fid\] \n"
	set e "\t set src(\$i) \[\$tcp(\$i) attach-source $src\] \n"
	set f "\t incr i \n \}"

	set str "$a_5 $a_4 $a_1 $a $b $c $d_1 $d $e $f"
	return $str
}

# check for commonalities bet this and generate-src-start.. X
AgentGen instproc generate-srm-patterns { join leave num \
		start stop srcnum } {
	#puts "generate srm patterns $join $leave $num $start \
	#	$stop $srcnum"

	# for now ignore the leave and stop until I know if
	# if they apply to srm agents or not !! XXX
	
	set ex {regexp {^([0-9]+)[\-]([0-9]+)$}}
	set strstart "" ;
	set done(start) 0;
	set ext "-source"
	foreach event { start } {
	  if { [eval "$ex [set $event] all begin end"] } {
		set str [$self randomize-time $begin $end $srcnum]
set a_5 "\n # randomize $event times in the \[$begin,$end\] range\n"
set a_4 "# for $srcnum srm sources. \n"
		set a "\n set i 0 \n"
		set ab "set times \{$str\} \n"
		set b "foreach j \[array names srmsrc\] \{ \n"
		set bb "\t set time \[lindex \$times \$i\] \n"
		set c "\t \$ns at \$time \"\$srmsrc(\$j) $event$ext\" \n"
		set d "\t incr i \n \} \n"
		set str$event "$a_5 $a_4 $a $ab $b $bb $c $d"
		set done($event) 1
	  } 
	}

	if { !$done(start) } {
set a_1 " # start all $srcnum srm sources at $start. \n"
		set a "foreach i \[array names srmsrc\] \{ \n"
set b "\t \$ns at $start \"\$srmsrc(\$i) start-source\" \n \} \n"
		set strstart "$a_1 $a $b"
	}

	if { [eval "$ex $join all begin end"] } {
		set str [$self randomize-time $begin $end $num]
		set srcNodes [lrange $str 0 \
			[expr [array size srmsrc] -1]]
		set rcvrNodes [lrange $str [array size srmsrc] end]


set a_1 "\n # randomize the join times for $num srm agents in\n" 
set a_2 "# the range \[$begin,$end\] \n"
		set a "\n set i 0 \n"
		set ab "set times \{$str\} \n"
		set b "foreach j \[array names srmsrc\] \{ \n"
		set bb "\t set time \[lindex \$times \$i\] \n"
		set c "\t \$ns at \$time \"\$srmsrc(\$j) start\" \n"
		set d "\t incr i \n \} \n"

		set e "foreach j \[array names srm\] \{ \n"
		set f "\t set time \[lindex \$times \$i\] \n"
		set g "\t \$ns at \$time \"\$srm(\$j) start\" \n"
		set h "\t incr i \n \} \n"

	set strjoin "$a_1 $a_2 $a $ab $b $bb $c $d $e $f $g $h"
	} else {
		set a_1 "# srm agents join at $join\n"
		set a "foreach j \[array names srm\] \{ \n"
		set b "\t \$ns at $join \"\$srm(\$j) start\" \n \} \n"
		set c "foreach j \[array names srmsrc\] \{ \n"
	set d "\t \$ns at $join \"\$srmsrc(\$j) start\" \n \} \n"
		set strjoin "$a_1 $a $b $c $d"
	}

	return "$strstart $strjoin"
}

AgentGen instproc generate-src-start { start stop num } {
	# puts "generate src $start $stop $num"
	set ex {regexp {^([0-9]+)[\-]([0-9]+)$}}
	set strstart "" ; set strstop ""
	set done(start) 0; set done(stop) 0
	# assume the start and stop intervals don't overlap,
	# we do not check if start is before stop for an agent XXX
	foreach event { start stop } {
	  if { [eval "$ex [set $event] all begin end"] } {
		set str [$self randomize-time $begin $end $num]
set a_2 "\n # randomize the $event time for the $num sources in\n" 
set a_1 "# the range \[$begin,$end\]\n"
		set a "\n set i 0 \n"
		set b "foreach time \{$str\} \{ \n"
		set c "\t \$ns at \$time \"\$src(\$i) $event\" \n"
		set d "\t incr i \n \} \n"
		set str$event "$a_2 $a_1 $a $b $c $d"
		set done($event) 1
	  } 
	}
	if { $done(start) && ($done(stop) || !$stop) } {
		return "$strstart $strstop"
	}
	# may want to clear up the src array before this run of
	# code generation... to avoid left overs.. XXX
	set a "foreach i \[array names src\] \{ \n"
	set a_1 ""; set a_2 ""
	if { !$done(start) } {
		set a_2 "# start the sources at $start \n"
		set b "\t \$ns at $start \"\$src(\$i) start\" \n"
	} else { set b "" }
	if { !$done(stop) && $stop } {
		set a_1 "# stop the sources at $stop \n"
		set c "\t \$ns at $stop \"\$src(\$i) stop\" \n"
	} else { set c "" }
	set d "\}"

	set str "$a_2 $a_1 $a $b $c $d"
	return "$strstart $strstop $str"
}

# the following takes as input the interval (first, last) from
# which distinct 'number' of ints are randomly chosen and returned.
# It is assumed that number <= interval
AgentGen instproc randomize-agents { first last number } {
	set interval [expr $last - $first]
	set result ""

	set maxrval [expr pow(2,31)]
	set intrval [expr $interval/$maxrval]
	for { set i 0 } { $i < $number } { incr i } {
		set randval [expr [ns-random] * $intrval]
		set randNode [expr int($randval) + $first]
		if { [info exists done($randNode)] } {
			set i [expr $i - 1]
		} else {
			set done($randNode) 1
			lappend result $randNode
		}
	}
	return $result
}

# takes in src and dst ranges (first,last) from which 'number' of
# distinct [i.e. src != dst] src,dst pairs are chosen
# the result is a list of pairs
AgentGen instproc randomize-agent-pairs { srcfirst srclast \
		dstfirst dstlast number	} {
	set srcinterval [expr $srclast - $srcfirst]
	set dstinterval [expr $dstlast - $dstfirst]
	set result ""
	set maxrval [expr pow(2,31)]
	set srcintrval [expr $srcinterval/$maxrval]
	set dstintrval [expr $dstinterval/$maxrval]
	for { set i 0 } { $i < $number } { incr i } {
		set srcrandval [expr [ns-random] * $srcintrval]
		set srcrandNode [expr int($srcrandval) + $srcfirst]
		while { 1 } {
		  set dstrandval [expr [ns-random] * $dstintrval]
		  set dstrandNode [expr int($dstrandval) + $dstfirst]
		  if { $srcrandNode != $dstrandNode } {
			break
		  }
		}
		lappend result [list $srcrandNode $dstrandNode]
	}
	return $result
}

AgentGen instproc randomize-time { first last number } {
	set times ""
	set interval [expr $last - $first]
	set maxrval [expr pow(2,31)]
	set intrval [expr $interval/$maxrval]

	for { set i 0 } { $i < $number } { incr i } {
		set randtime [expr ([ns-random] * $intrval) + $first]
		# XXX include only 6 decimals (i.e. usec)
		lappend times [format "%.6f" $randtime]
	}
	return $times
}

proc mark-end { file } {
	global AllCommandLines
	# puts "mark end of file $file"
	if { [catch {set f [open $file a]}] } {
		puts "can't append to file $file"
	}

	set a_1 " \n \} ; # ending the generate-agents proc.\n\n"
	set a "############## End of file marker ##########\n"
	set b "# Following are the command lines used:"
	puts $f "$a_1 $a $b"
	foreach cmdLine $AllCommandLines {
		puts $f " # $cmdLine"
	}
	flush $f
	close $f
}
