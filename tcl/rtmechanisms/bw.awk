#
# bw.awk
#       awk script to parse the output of a ns trace file,
#       and produce mean bandwidth vs time-interval
#
# Usage:
#       awk -f bw.awk OP=<offered|achieved> [args] tracefile
#
# [args] can be:
#       FID=<flowid number>
#       PTYPE=<packet-type>
#	INTERVAL=<averaging-interval-in-secs>
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
	# other variables
	LAST=0
	TOTBYTES=0
	INTERVAL=1.0
}
/^+/ && OP == "offered"  {
	if ( FID != "" && FID != $FFID )
		next
	if ( PTYPE != "" && PTYPE != $FPTYPE )
		next
	TOTBYTES += $FPLEN

	}

/^-/ && OP == "achieved"  {
	if ( FID != "" && FID != $FFID )
		next
	if ( PTYPE != "" && PTYPE != $FPTYPE )
		next
	TOTBYTES += $FPLEN

	}

	{
		ELAPSED=$FTIME-LAST
		if ( ELAPSED >= INTERVAL ) {
			LAST=$FTIME
			print $FTIME, TOTBYTES / ELAPSED
			TOTBYTES=0
		}
	}

OP == "" {
	print "Usage: awk -f bw.awk OP=<offered|achieved> ... tracefile"
	print "  Optional vars: FID=<fid> PTYPE=<tcp|ack|cbr|...> INTERVAL=<averaging interval>"
	exit 1
}
