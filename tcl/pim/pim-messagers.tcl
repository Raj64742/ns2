Class Agent/Message/pim -superclass Agent/Message

# message handler [decode headers.. dispatch.. etc]
Agent/Message/pim instproc handle msg {
	$self instvar proto
	set L [split $msg /]
	set type [lindex $L 0]
	set L [lreplace $L 0 0]
	switch $type {
		103 { $proto recv-hello $L }
		104 { $proto recv-register $L }
		105 { $proto recv-register-stop $L }
		106 { $proto recv-join $L }
		112 { $proto recv-prune $L }
		107 { $proto recv-bootstrap $L }
		108 { $proto recv-assert $L }
		109 -
		110 { puts "GRAFTS IN SM !!!!" }
		111 { $proto recv-c-rp-adv $L }
		default { puts "unknown pim-sm type" }
	}
}

Class Agent/Message/reg -superclass Agent/Message

Agent/Message/reg instproc init { src grp rp protocol } {
	$self next
	$self instvar source group RP proto
	set source $src
	set group $grp
	set RP $rp
	set proto $protocol
	$self set class_ [PIM set REGISTER]
}

Agent/Message/reg instproc handle msg {
	$self instvar source group proto
	set type [PIM set REGISTER]
	set mysrcAdd [expr [[$proto set Node] id] << 8 | [$self port]]
	set origsrcAdd $source
	set nullBit 0
	set borderBit 0
#$self send "$type/$mysrcAdd/$origsrcAdd/$group/$nullBit/$borderBit/$msg"
$self send "$type/$mysrcAdd/$origsrcAdd/$group/$nullBit/$borderBit"

}


