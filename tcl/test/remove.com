#!/bin/sh
# To remove temporary files in tcl/test.
# To run: "./remove.com"
#
rm -f temp* *.ps
rm -f t?.tcl
rm -f *.tr *.tr1 *.nam *.xgr
rm -f all.*
rm -f fairflow.*
rm -f t t?
rm -f chart? 
for i in simple tcp full monitor red sack schedule cbq red-v1 cbq-v1 sack-v1 \
  v1 \
  vegas-v1 rbp tcp-init-win tcpVariants ecn manual-routing hier-routing \
  intserv webcache mcast newreno srm session mixmode algo-routing vc \
  simultaneous lan wireless-lan ecn-ack mip energy wireless-gridkeeper mcache \
  satellite wireless-lan-newnode wireless-lan-aodv WLtutorial aimd greis \
  rfc793edu friendly rfc2581
do
	echo test-output-$i
	rm -f test-output-$i/*.test
	rm -f test-output-$i/*[a-z,A-R,T-Y,0-9]
done


