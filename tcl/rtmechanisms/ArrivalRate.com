# plot arrival rate vs. drops
set filename=$1
set label=$2
rm -f chart chart1
awk '{if($1>0&&$1!~/:/){print}}' $filename > chart
awk '(NF==2){print}' chart > chart2
#
s << !
chart _ matrix(scan("chart2"),ncol=2,byr=T)
xr _ range(chart[,1])
yr _ range(0,chart[,2])
postscript("tests.ps", horizontal=F, width = 4.5, height = 2.5, colors = 0:1)
par(mgp=c(1,0,0))
par(mar=c(3,2,1,0))
par(tck=0.02)
par(pch=".")
par(cex=0.6)
plot(chart[,1],chart[,2],xlim=xr,ylim=yr,type="n",
xlab="Time",
ylab="Arrival Rate (Fraction of Bytes)",
sub="(For the highest-bandwidth flow in each reporting interval)")
lines(chart[,1],chart[,2],type="l")
q()
!
cp tests.ps ArrivalRate.$label.ps
