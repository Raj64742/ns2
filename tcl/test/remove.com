# To remove temporary files in tcl/test.
# To run: "csh remove.com"
#
rm -f temp* *.ps
rm -f t?.tcl
rm -f *.tr *.tr1 *.nam *.xgr
rm -f all.*
rm -f fairflow.*
rm -f t t?
rm -f chart? 
@ count = 1
again:
t1: 
set directory=test-output-red
goto doit
t2:
set directory=test-output-cbq-v1
goto doit
t3:
set directory=test-output-sack
goto doit
t4:
set directory=test-output-tcp
goto doit
t5:
set directory=test-output-mcast
goto doit
t6:
set directory=test-output-vegas-v1
goto doit
t7:
set directory=test-output-v1
goto doit
t8:
set directory=test-output-rbp
goto doit
t9:
set directory=test-output-cbq
goto doit
t10:
set directory=test-output-schedule
goto doit
t11:
set directory=test-output-sack-v1
goto doit
t12:
set directory=test-output-full
goto doit
t13:
set directory=test-output-ecn
goto doit
t14:
set directory=test-output-manual-routing
goto doit
t15:
set directory=test-output-tcpVariants
goto doit
t16:
set directory=test-output-srm
goto doit
t17:
set directory=test-output-session
goto doit
t18:
set directory=test-output-tcp-init-win
goto doit
t19:
set directory=test-output-simple
goto doit
t20:
set directory=test-output-lan
goto doit
t21:
set directory=test-output-ecn-ack
goto doit
t22:
set directory=test-output-algo-routing
goto doit
t23:
set directory=test-output-hier-routing
goto doit
t24:
set directory=test-output-vc
goto doit
t25:
set directory=test-output-wireless-lan
goto doit
t26:
set directory=test-output-ecn-ack
goto doit
t27:
set directory=test-output-ecn-ack
goto doit
t28:
set directory=test-output-intserv
goto doit
t29: 
set directory=test-output-mcache
goto doit
t30: 
set directory=test-output-mip
goto doit
t31: 
set directory=test-output-wireless-gridkeeper
goto doit
t32: 
set directory=test-output-friendly
goto doit
t33: 
set directory=test-output-satellite
goto doit
t34:
set directory=test-output-aimd
goto doit
t35: 
exit
doit:
echo $directory
rm -f $directory/*.test
rm -f $directory/*[a-z,A-R,T-Y,0-9]
@ count ++
goto t$count


