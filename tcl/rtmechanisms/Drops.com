# plot num. drops vs. time
## csh Drops.com fairflow.rpt.drop fairflow.rpt.forced 1
set filename=$1
set filename2=$2
set num=$3
rm -f chart chart2
awk '{if($1>0&&$1!~/:/){print};if($1~/flow/){print " "}}' $filename > chart
awk '(NF==2){print}' chart > chart2
mv chart2 chart
rm -f chart1 
awk '{if($1>0&&$1!~/:/){print};if($1~/flow/){print " "}}' $filename2 > chart1
awk '(NF==2){print}' chart1 > chart2
mv chart2 chart1
#
s << !
chart _ matrix(scan("chart"),ncol=2,byr=T)
chart1 _ matrix(scan("chart1"),ncol=2,byr=T)
xr _ range(chart[,1])
yr _ range(0,chart[,2])
postscript("tests.ps", horizontal=F, width = 4.5, height = 2.5, colors = 0:1)
par(mgp=c(1,0,0))
par(mar=c(3,2,0,0))
par(tck=0.02)
par(pch=".")
par(cex=0.6)
plot(chart[,1],chart[,2],xlim=xr,ylim=yr,type="n",
xlab="Time",
ylab="Number of Drops in Sample",
sub="(Dotted line: Number of Forced Drops)")
lines(chart[,1],chart[,2],type="l")
points(chart[,1],chart[,2],type="p")
par(lty=3)
lines(chart1[,1],chart1[,2],type="l")
points(chart1[,1],chart1[,2],type="p")
q()
!
cp tests.ps Drops.$num.ps
