# plot distribution of ratio between combined metric/arrival rate. 
## csh Dist.com Dist.sum.data $total
set filename=$1
set num=$2
set total=$3
#
awk '{print $1, $2/'$total} $filename > chart
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
plot(chart[,1],chart[,2],xlim=c(0,2),ylim=yr,type="n",
xlab="Combined Metric/Fraction of Arrivals",
ylab="Density")
lines(chart[,1],chart[,2],type="l")
points(chart[,1],chart[,2],type="p")
q()
!
cp tests.ps Dist.$num.ps
