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
        switch $code {
                "CACHE_MISS" { $self handle-cache-miss $argslist }
                "WRONG_IIF" { $self handle-wrong-iif $argslist }  
                default { puts "no match for upcall, code is $code" }
        }
}
 
McastProtocol instproc handle-wrong-iif { argslist } {

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
