CMUTrace instproc init { tname type } {
	$self next $tname $type
	$self instvar type_ src_ dst_ callback_ show_tcphdr_

	set type_ $type
	set src_ 0
	set dst_ 0
	set callback_ 0
	set show_tcphdr_ 0
}

CMUTrace instproc attach fp {
	$self instvar fp_
	set fp_ $fp
	$self cmd attach $fp_
}

Class CMUTrace/Send -superclass CMUTrace
CMUTrace/Send instproc init { tname } {
	$self next $tname "s"
}

Class CMUTrace/Recv -superclass CMUTrace
CMUTrace/Recv instproc init { tname } {
	$self next $tname "r"
}

Class CMUTrace/Drop -superclass CMUTrace
CMUTrace/Drop instproc init { tname } {
	$self next $tname "D"
}


CMUTrace/Recv set src_ 0
CMUTrace/Recv set dst_ 0
CMUTrace/Recv set callback_ 0
CMUTrace/Recv set show_tcphdr_ 0
CMUTrace/Recv set off_sr_ 0

CMUTrace/Send set src_ 0
CMUTrace/Send set dst_ 0
CMUTrace/Send set callback_ 0
CMUTrace/Send set show_tcphdr_ 0
CMUTrace/Send set off_sr_ 0

CMUTrace/Drop set src_ 0
CMUTrace/Drop set dst_ 0
CMUTrace/Drop set callback_ 0
CMUTrace/Drop set show_tcphdr_ 0
CMUTrace/Drop set off_sr_ 0
CMUTrace/Drop set off_SR_ 0
CMUTrace/Drop set off_arp_ 0
