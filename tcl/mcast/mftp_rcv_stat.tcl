# mftp_rcv_stat.tcl
# Author:		Christoph Haenle <haenle@telematik.informatik.uni-karlsruhe.de>
# Version Date:	...
# 

Class Agent/MFTP/Rcv/Stat -superclass Agent/MFTP/Rcv

Agent/MFTP/Rcv/Stat instproc init { } {
    $self instvar nb_useful_recv nb_full_disc nb_lin_dep_disc

    $self next
    foreach var [list nb_useful_recv nb_full_disc nb_lin_dep_disc] {
        set $var 0
    }
}

Agent/MFTP/Rcv/Stat instproc recv-useful { CurrentPass CurrentGroup CwPat } {
    $self instvar nb_useful_recv

    puts stdout "recv-useful!"
    $self next $CurrentPass $CurrentGroup $CwPat

    incr nb_useful_recv
}


Agent/MFTP/Rcv/Stat instproc recv-group-full { CurrentPass CurrentGroup CwPat } {
    $self instvar nb_full_disc

    puts stdout "recv-group-full!"
    $self next $CurrentPass $CurrentGroup $CwPat

    incr nb_full_disc
}


Agent/MFTP/Rcv/Stat instproc recv-dependent { CurrentPass CurrentGroup CwPat } {
    $self instvar nb_lin_dep_disc

    puts stdout "recv-dependent!"
    $self next $CurrentPass $CurrentGroup $CwPat

    incr nb_lin_dep_disc
}

Agent/MFTP/Rcv/Stat instproc done-notify { args } {
    $self instvar nb_useful_recv nb_full_disc nb_lin_dep_disc
    eval $self next $args $nb_useful_recv $nb_full_disc $nb_lin_dep_disc
}
