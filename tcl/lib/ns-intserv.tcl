#
# Copyright (c) Xerox Corporation 1997. All rights reserved.
#
# License is granted to copy, to use, and to make and to use derivative
# works for research and evaluation purposes, provided that Xerox is
# acknowledged in all documentation pertaining to any such copy or
# derivative work. Xerox grants no other licenses expressed or
# implied. The Xerox trade name should not be used in any advertising
# without its written permission. 
#
# XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
# MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
# FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
# express or implied warranty of any kind.
#
# These notices must be retained in any copies of any part of this
# software. 
#
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-intserv.tcl,v 1.11 2000/08/30 23:27:51 haoboy Exp $

#defaults
Queue/SimpleIntServ set qlimit1_ 50
Queue/SimpleIntServ set qlimit0_ 50

Agent/SA set rate_ 0
Agent/SA set bucket_ 0
Agent/SA set packetSize_ 210

ADC set backoff_ true
ADC set dobump_ true
ADC/MS set backoff_ false

ADC set src_ -1
ADC set dst_ -1
ADC/MS set utilization_ 0.95
ADC/MSPK set utilization_ 0.95
ADC/Param set utilization_ 1.0
ADC/HB set epsilon_ 0.7
ADC/ACTO set s_ 0.002
ADC/ACTO set dobump_ false
ADC/ACTP set s_ 0.002
ADC/ACTP set dobump_ false


Est/TimeWindow set T_ 3
Est/ExpAvg set w_ 0.125
Est set period_ 0.5

ADC set bandwidth_ 0

SALink set src_ -1
SALink set dst_ -1

Est set src_ -1
Est set dst_ -1


Class IntServLink -superclass  SimpleLink
IntServLink instproc init { src dst bw delay q arg {lltype "DelayLink"} } {
	
	$self next $src $dst $bw $delay $q $lltype ; # SimpleLink ctor
	$self instvar queue_ link_

	$self instvar measclassifier_ signalmod_ adc_ est_ measmod_
	
	set ns_ [Simulator instance]
	
	#Create a suitable adc unit from larg with suitable params
	set adctype [lindex $arg 3]
	set adc_ [new ADC/$adctype]
	$adc_ set bandwidth_ $bw
	$adc_ set src_ [$src id]
	$adc_ set dst_ [$dst id]
	
	if { [lindex $arg 5] == "CL" } {
		#Create a suitable est unit 
		set esttype [lindex $arg 4]
		set est_ [new Est/$esttype]
		$est_ set src_ [$src id]
		$est_ set dst_ [$dst id]
		$adc_ attach-est $est_ 1
		
		#Create a Measurement Module 
		set measmod_ [new MeasureMod]
		$measmod_ target $queue_
		$adc_ attach-measmod $measmod_ 1
	}
	
	#Create the signalmodule
	set signaltype [lindex $arg 2]
	set signalmod_ [new $signaltype]
	$signalmod_ set src_ [$src id]
	$signalmod_ set dst_ [$dst id]
	$signalmod_ attach-adc $adc_
	$self add-to-head $signalmod_


	#Create a measurement classifier to decide which packets to measure
	$self create-meas-classifier
	$signalmod_ target $measclassifier_
	
	#Schedule to start the admission control object
	$ns_ at 0.0 "$adc_ start"
}
IntServLink instproc buffersize { b } {
	$self instvar est_ adc_
	$est_ setbuf [set b]
	$adc_ setbuf [set b]
}


#measClassifier is an instance of Classifier/Hash/Flow
#for right now
# FlowId 0 -> Best Effort traffic
# FlowId non-zero -> Int Serv traffic 

IntServLink instproc create-meas-classifier {} {
	$self instvar measclassifier_ measmod_ link_ queue_
	
	set measclassifier_ [new Classifier/Hash/Fid 1 ]
	#set slots for measclassifier
	set slot [$measclassifier_ installNext $queue_]
	$measclassifier_ set-hash auto 0 0 0 $slot 
	
	#Currently measure all flows with fid.ne.0 alone
	set slot [$measclassifier_ installNext $measmod_]
	$measclassifier_ set default_ 1
}

IntServLink instproc trace-sig { f } {
	$self instvar signalmod_ est_ adc_
	$signalmod_ attach $f
	$est_ attach $f
	$adc_ attach $f
	set ns [Simulator instance]
	$ns at 0.0 "$signalmod_ add-trace"
}

#Helper  function to output link utilization and bw estimate
IntServLink instproc trace-util { interval {f ""}} {
	$self instvar est_
	set ns [Simulator instance]
	if { $f != "" } {
		puts $f "[$ns now] [$est_ load-est] [$est_ link-utlzn]" 
	}
	$ns at [expr [$ns now]+$interval] "$self trace-util $interval $f" 
}
