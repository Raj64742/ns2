#
# pkts.awk
# 	awk script to parse the output of a ns trace file,
#	and produce seqno vs time XY data file
#
# Usage:
#	awk -f pkts.awk OP=<arrivals|drops> [args] tracefile
#
# [args] can be:
#	FID=<flowid number>
#	PTYPE=<packet-type>
#
#
BEGIN {
	# field numbers
	FTIME=2
	FPTYPE=5
	FPLEN=6
	FFID=8
	FSEQ=11
	FUID=12
}
/^+/ && OP == "arrivals"  {
	if ( FID != "" && FID != $FFID )
		next
	if ( PTYPE != "" && PTYPE != $FPTYPE )
		next
	print $FTIME, $FSEQ
}

/^d/ && OP == "drops"  {
	if ( FID != "" && FID != $FFID )
		next
	if ( PTYPE != "" && PTYPE != $FPTYPE )
		next
	print $FTIME, $FSEQ
}

OP == "" {
	print "Usage: awk -f pkts.awk OP=<arrivals|drops> ... tracefile"
	print "  Optional vars: FID=<fid> PTYPE=<tcp|ack|cbr|...>"
	exit 1
}
