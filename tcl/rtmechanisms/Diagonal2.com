# plot arrival rate vs. drops
set filename=$1
set num=$2
set label=$3
set label1=$4
rm -f chart chart1
awk '{if($1>0&&$1!~/:/){print};if($1~/flow/){print " "}}' $filename > chart
awk '{if($1~/flow/){flow=$2};if($1>0&&$1!~/:/){print flow};}' $filename > chart1
awk '(NF==2){print}' chart > chart2
#
s << !
chart _ matrix(scan("chart2"),ncol=2,byr=T)
chart1 _ matrix(scan("chart1"),ncol=1,byr=T)
xr _ range(chart[,1])
yr _ range(chart[,2])
postscript("tests.ps", horizontal=F, width = 4.5, height = 3.2, colors = 0:1)
par(mgp=c(1,0,0))
par(mar=c(3,2,0,0))
par(tck=0.02)
par(pch=".")
par(cex=0.6)
plot(chart[,1],chart[,2],xlim=c(0,100),ylim=c(0,100),type="n",
xlab="Per-Flow Arrival Rate(%)",
ylab="Flow's Percent of Drops ($label)",
sub="($label Drop Metric, for a Drop-Tail Queue Measured in $label1)")
text(chart[,1],chart[,2],label=chart1[])
par(lty=3)
lines(c(0,100),c(0,100),type="l")
q()
!
cp tests.ps Diagonal.$num.ps
