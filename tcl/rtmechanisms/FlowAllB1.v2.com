# To run this:  "csh FlowAllB1.v2.com".
#
set randomseed=0
set filename=fairflow.xgr
set run=20
rm -f Dist.data
set num = 0
while ($num < 100)
  ../../ns Flow4B.v2.tcl combined $randomseed
  awk '{print $2}' fairflow.xgr.c.4 >> Dist.data
  @ num++
end
##
awk '(NF==1){print}' Dist.data | sort  | uniq -c | awk '{print $2, $1}' >\
  Dist.sum.$run.data
#set total=`awk '{sum+=$2}END{print sum}' Dist.sum.$run.data`  
#
set total=1761
csh Dist.com Dist.sum.$run.data $run $total
cp Dist.$run.ps ~/paper/red/penalty/figures
