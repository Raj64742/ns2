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

McastProtocol instproc getType {} {
        $self instvar type
        return $type
}

McastProtocol instproc init {} {
        $self instvar status
        set status "down"
}

McastProtocol instproc start {} {
        $self instvar status
        set status "up"
}

McastProtocol instproc getStatus {} {
        $self instvar status
        return $status
}

McastProtocol instproc upcall { argslist } {
        set code [lindex $argslist 0]
        set argslist [lreplace $argslist 0 0]
        # puts "does this upcalled"
        switch $code {
                "CACHE_MISS" { $self handle-cache-miss $argslist }
                "WRONG_IIF" { $self handle-wrong-iif $argslist }  
                default { puts "no match for upcall, code is $code" }
        }
}
 
McastProtocol instproc handle-wrong-iif { argslist } {

}

McastProtocol instproc join-group {} {
        $self instvar dynT_ Node group_
        if [info exists dynT_] {
	    foreach tr $dynT_ {
		$tr annotate "[$Node id] join group $group_"
	    }
	}
}

McastProtocol instproc leave-group {} {
        $self instvar dynT_ Node group_
        if [info exists dynT_] {
	    foreach tr $dynT_ {
		$tr annotate "[$Node id] leave group $group_"
	    }
	}
}

McastProtocol instproc trace-dynamics { ns f src } {
        $self instvar dynT_
        lappend dynT_ [$ns create-trace Generic $f $src $src]
}

McastProtocol instproc trace { ns f src } {
	$self trace-dynamics $ns $f $src
}

McastProtocol instproc notify changes {
	# no-op
}

#############################################
Class McastProtoArbiter

# initialize with a list of the mcast protocols
McastProtoArbiter instproc init { protos } {
        $self next
        $self instvar protocols
        set protocols $protos
}

McastProtoArbiter instproc addproto { proto } {
        $self instvar protocols
        lappend protocols $proto
}

McastProtoArbiter instproc getType { protocolType } {
        $self instvar protocols
        foreach proto $protocols {
                if { [$proto getType] == $protocolType } {
                        return $proto
                }
        }
        return -1
}

McastProtoArbiter instproc start {} {
        $self instvar protocols
        foreach proto $protocols {
                $proto start
        }
}

McastProtoArbiter instproc notify changes {
	$self instvar protocols
	foreach proto $protocols {
		$proto notify $changes
	}
}

# similar to membership indication by igmp.. 
McastProtoArbiter instproc join-group { group } {
        $self instvar protocols
        foreach proto $protocols {
                $proto join-group $group
        }
}

McastProtoArbiter instproc leave-group { group } {
        $self instvar protocols
        foreach proto $protocols {
                $proto leave-group $group
        }
}

McastProtoArbiter instproc upcall { argslist } {
        $self instvar protocols
        foreach proto $protocols {
                $proto upcall $argslist
        }
}

McastProtoArbiter instproc drop { replicator src dst } {
        $self instvar protocols
        foreach proto $protocols {
                $proto drop $replicator $src $dst
        }
}
