#!/bin/sh

if [ -z $1 ]; then
 GP="off"; QUIET=1
else 
  if [ $1 = "gp" ]; then
     GP="on"; QUIET=1 
  fi
  if [ $1 = "xgraph" ]; then
     GP="off"; QUIET=0
  fi
fi


# Fig: WLAN duplex
ns mtp.tcl -type wlan_duplex -stop 10 -quiet $QUIET
./timeseq.cmd wlan_duplex
if [ $GP = "on" ]; then gv wlan_duplex.eps; fi

# Fig: WLAN duplex, 1Mbps
ns mtp.tcl -type wlan_duplex -stop 10 -quiet $QUIET -bwUL 1000000 \
-bwDL 1000000
./timeseq.cmd wlan_duplex_1mb
if [ $GP = "on" ]; then gv wlan_duplex_1mb.eps; fi

# Fig: WLAN duplex with reverse TCP
ns mtp.tcl -type wlan_duplex -flowsRev 1 -stop 10 -quiet $QUIET
./timeseq.cmd wlan_duplex_rev
if [ $GP = "on" ]; then gv wlan_duplex_rev.eps; fi

# Fig: WLAN complex
ns mtp.tcl -type wlan_complex -stop 10 -quiet $QUIET  
./timeseq.cmd wlan_complex 
if [ $GP = "on" ]; then gv wlan_complex.eps; fi

# Fig: WLAN complex with reverse TCP
ns mtp.tcl -type wlan_complex -flowsRev 1 -stop 10 -quiet $QUIET
./timeseq.cmd wlan_complex_rev
if [ $GP = "on" ]; then gv wlan_complex_rev.eps; fi

# Fig: WLAN as 802.3 with reverse TCP
ns mtp.tcl -type wlan_ether -flowsRev 1 -stop 10 -quiet $QUIET
./timeseq.cmd wlan_ether_rev
if [ $GP = "on" ]; then gv wlan_ether_rev.eps; fi

# Fig: GPRS Latency 500ms
ns mtp.tcl -flows 0 -pingInt 0.5 -stop 500 -allocLenDL 'U(0.16,0.19)' \
-allocHoldDL 'U(2,5)' -allocLenUL 'U(0.5,0.6)' \
-allocHoldUL 'U(0.01,0.4)' -quiet $QUIET
./pingplot.cmd sim-latency-500ms
if [ $GP = "on" ]; then gv sim-latency-500ms.eps; fi

# Fig: GPRS Latency 5s
ns mtp.tcl -flows 0 -pingInt 5 -stop 5000 -allocLenDL 'U(0.16,0.19)' \
-allocHoldDL 'U(2,5)' -allocLenUL 'U(0.5,0.6)' \
-allocHoldUL 'U(0.01,0.4)' -quiet $QUIET
./pingplot.cmd sim-latency-5000ms
if [ $GP = "on" ]; then gv sim-latency-5000ms.eps; fi

# Fig: delay jitter due to handovers
ns mtp.tcl -delayInt 'E(3)' -delayLen 'E(3)' -quiet $QUIET
./timeseq.cmd spike-delays
if [ $GP = "on" ]; then gv spike-delays.eps; fi

# Fig: delay jitter due to ARQ
ns mtp.tcl -delayInt 'E(0.3)' -delayLen 'E(0.3)' -quiet $QUIET
./timeseq.cmd arq-delays
if [ $GP = "on" ]; then gv arq-delays.eps; fi

# Fig: Vertical handover from GPRS to WLAN
ns mtp.tcl -flows 0 -flowsTfrc 1 -vhoTarget wlan_duplex -quiet $QUIET \
-stop 60 
./timeseq.cmd vho-tfrc
if [ $GP = "on" ]; then gv vho-tfrc.eps; fi

# Fig: Second vertical handover from GPRS to WLAN
ns mtp.tcl -flows 0 -flowsTfrc 1 -vhoTarget wlan_duplex -quiet $QUIET \
-stop 60 -gprsbuf 30 -wlan_duplex_buf 100 -tfrcFB 1
./timeseq.cmd vho-tfrc-ovb-fb1
if [ $GP = "on" ]; then gv vho-tfrc-ovb-fb1.eps; fi
# Running this one with: Agent/TFRCSink set NumFeedback_ 3
# The sender increases its sending rate slighly faster.
# The sender has drops from 11-16 seconds.
# The sender starts slowing down.
# It takes a long time for the sender to forget about this old history.

ns mtp.tcl -flows 0 -flowsTfrc 1 -vhoTarget wlan_duplex -quiet $QUIET \
-stop 60 -gprsbuf 30 -wlan_duplex_buf 100 -tfrcFB 3
./timeseq.cmd vho-tfrc-ovb-fb3
if [ $GP = "on" ]; then gv vho-tfrc.eps-ovb-fb3; fi

