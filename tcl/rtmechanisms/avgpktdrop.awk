BEGIN {drops=0; arr=0}
{drops += $16; arr += $12}
END {print "drops = " drops; print "arrivals = " arr; print "avg pkt drop % = " drops/arr}
