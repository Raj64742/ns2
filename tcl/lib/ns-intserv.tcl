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

#defaults
Queue/SimpleIntServ set qlimit1_ 50
Queue/SimpleIntServ set qlimit0_ 50

Agent/CBR/UDP/SA set rate_ 0
Agent/CBR/UDP/SA set bucket_ 0

ADC/MS set utilization_ 0.95
ADC/HB set epsilon_ 0.7
ADC/ACTO set s_ 0.002
ADC/ACTP set s_ 0.002


Est/TimeWindow set T_ 3
Est/ExpAvg set w_ 0.125
Est set period_ 0.5

ADC set bandwidth_ 0

Class IntServLink -superclass  SimpleLink
IntServLink instproc init { src dst bw delay q arg {lltype "DelayLink"} } {
	
	$self next $src $dst $bw $delay $q $lltype ; # SimpleLink ctor
	$self instvar head_ queue_ link_
	
	$self instvar measclassifier_ signalmod_ adc_ est_ measmod_
	
	set ns_ [Simulator instance]
	
	#Create a suitable adc unit from larg with suitable params
	set adctype [lindex $arg 3]
	set adc_ [new ADC/$adctype]
	$adc_ set bandwidth_ $bw
	
	
	if { [lindex $arg 5] == "CL" } {
		#Create a suitable est unit 
		set esttype [lindex $arg 4]
		set est_ [new Est/$esttype]
		$adc_ attach-est $est_ 1
		
		#Create a Measurement Module 
		set measmod_ [new MeasureMod]
		$measmod_ target $link_
		$adc_ attach-measmod $measmod_ 1
	}
	
	#Create the signalmodule
	set signaltype [lindex $arg 2]
	set signalmod_ [new $signaltype]
	$signalmod_ target $queue_
	$signalmod_ attach-adc $adc_
	set head_ $signalmod_
	
	#Create a measurement classifier to decide which packets to measure
	$self create-meas-classifier
	$queue_ target $measclassifier_
	
	#Schedule to start the admission control object
	$ns_ at 0.0 "$adc_ start"
}


#measClassifier is an instance of Classifier/Hash/Flow
#for right now
# FlowId 0 -> Best Effort traffic
# FlowId non-zero -> Int Serv traffic 

IntServLink instproc create-meas-classifier {} {
	$self instvar measclassifier_ measmod_ link_
	
	set measclassifier_ [new Classifier/Hash/Fid 1 ]
	#set slots for measclassifier
	set slot [$measclassifier_ installNext $link_]
	$measclassifier_ set-hash 0 0 0 0 $slot 
	
	#Currently measure all flows with fid.ne.0 alone
	set slot [$measclassifier_ installNext $measmod_]
	$measclassifier_ set default_ 1
}

