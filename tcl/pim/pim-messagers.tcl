 #
 # tcl/pim/pim-messagers.tcl
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
 # Contributed by Ahmed Helmy, Ported by Polly Huang (USC/ISI), 
 # http://www-scf.usc.edu/~bhuang
 # 
 #
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
        $self next $msg
        $self instvar source group proto
        set type [PIM set REGISTER]
        set mysrcAdd [expr [[$proto set Node] id] << 8 | [$self port]]
        set origsrcAdd $source
        set nullBit 0
        set borderBit 0
        set mesg "$type/$mysrcAdd/$origsrcAdd/$group/$nullBit/$borderBit/$msg"
        $self transmit $mesg
}
