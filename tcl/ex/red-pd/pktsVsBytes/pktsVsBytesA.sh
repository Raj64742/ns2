#! /bin/csh

rm -f data* flows*
rm -f pktsVsBytes/flows* pktsVsBytes/tmp* pktsVsBytes/dropRate* pktsVsBytes/data*

set t1=1000
foreach try ( 2 )
    echo "Doing iteration $try, for RED in packet mode."
    foreach flows ( 4 8 12 16 20 )
	    echo "Doing flows ${flows}"
	    ns red-pd.tcl one netPktsVsBytesA PktsVsBytes flows ${flows} testUnresp 0 verbose -1 time $t1 enable 0
	    mv one.netPktsVsBytesA.PktsVsBytes.0.flows flowsA0-${try}-${flows}

	    ns red-pd.tcl one netPktsVsBytesA PktsVsBytes flows ${flows} testUnresp 0 verbose -1 time $t1
	    mv one.netPktsVsBytesA.PktsVsBytes.1.flows flowsA1-${try}-${flows}

end
foreach try ( 1 )
    echo "Doing iteration $try, for RED in byte mode."
    foreach flows ( 4 8 12 16 20 )
	    echo "Doing flows ${flows}"
	    ns red-pd.tcl one netPktsVsBytes PktsVsBytes flows ${flows} testUnresp 0 verbose -1 time $t1 enable 0
	    mv one.netPktsVsBytesA.PktsVsBytes.0.flows flows0-${try}-${flows}

	    ns red-pd.tcl one netPktsVsBytes PktsVsBytes flows ${flows} testUnresp 0 verbose -1 time $t1
	    mv one.netPktsVsBytesA.PktsVsBytes.1.flows flows1-${try}-${flows}

end

pktsVsBytes/pktsVsBytesA.pl flowsA0 $t1 2 0
pktsVsBytes/pktsVsBytesA.pl flowsA1 $t1 2 1
pktsVsBytes/pktsVsBytesA.pl flows0 $t1 1 0
pktsVsBytes/pktsVsBytesA.pl flows1 $t1 1 1

mv flows* data* pktsVsBytes
cd pktsVsBytes
gnuplot pktsVsBytesA.gp
gv pktsVsBytes0-A.ps &
gv pktsVsBytes1-A.ps &
gv pktsVsBytes0.ps &
gv pktsVsBytes1.ps &

cd ..

