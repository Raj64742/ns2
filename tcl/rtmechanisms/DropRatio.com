# plot ratio of arriving packets dropped vs. time
## csh DropRatio.com fairflow2.xgr 1
set filename=$1
set num=$2
rm -f chart chart2
awk '{if($1>0&&$1!~/:/){print};if($1~/flow/){print " "}}' $filename > chart
awk '(NF==2){print}' chart > chart2
mv chart2 chart
#
s << !
chart _ matrix(scan("chart"),ncol=2,byr=T)
xr _ range(chart[,1])
yr _ range(0,chart[,2])
postscript("tests.ps", horizontal=F, width = 4.5, height = 2.5, colors = 0:1)
par(mgp=c(1,0,0))
par(mar=c(2,2,0,0))
par(tck=0.02)
par(pch=".")
par(cex=0.6)
plot(chart[,1],chart[,2],xlim=xr,ylim=yr,type="n",
xlab="Time",
ylab="Packets Dropped (Fraction of Arrivals)")
lines(chart[,1],chart[,2],type="l")
points(chart[,1],chart[,2],type="p")
q()
!
cp tests.ps DropFrac.$num.ps
