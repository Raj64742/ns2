Class NamSimulator -superclass Simulator

NamSimulator instproc init { outfile } {
	$self next
	$self set chan_ [open $outfile w]
	
	$self set nam_config_ "proc nam_config {net} \{\n"
}


NamSimulator instproc node {} {
	set n [$self next]

	set namcmd "\t\$net node [$n id] circle\n"
	$self instvar nam_config_
	set nam_config_ $nam_config_$namcmd
	
	return $n
}

NamSimulator instproc simplex-link { n1 n2 bw delay type } {
	$self next $n1 $n2 $bw $delay $type
	set id1 [$n1 id]
	set id2 [$n2 id]

	$self instvar nam_config_ link_mark_
	if { ![info exists link_mark_($id2,$id1)] } {
		set namcmd "\tauto_mklink \$net $id1 $id2 $bw $delay \n"
		set nam_config_ $nam_config_$namcmd
	}

	set link_mark_($id1,$id2) 1

	set namcmd "\t\$net queue $id1 $id2 0.5 \n"
	set nam_config_ $nam_config_$namcmd
}


NamSimulator instproc config_dump {} {
	$self instvar nam_config_
	set nam_config_ $nam_config_\}\n
	
	$self instvar chan_
	puts $chan_ $nam_config_
	close $chan_
}
