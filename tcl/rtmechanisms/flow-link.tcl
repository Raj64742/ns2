#
# create a link with flow monitoring capability
#

CBQLink instproc flowmon {} {
	$self instvar classifier_ head_
	if { [info exists classifier_] } {
puts "classifier exists"
		delete $classifier_
	}
	set classifier_ [new Classifier/Hash/SrcDestFid 33]
	set head_ $classifier_
}
