{
time = $2;
saddr = $4;
sport = $6;
daddr = $8;
dport = $10;
hiack = $14;
cwnd = $18;
ssthresh = $20;
srtt = $26;
rttvar = $28;

if (!((saddr, sport, daddr, dport) in starttime)) {
	starttime[saddr, sport, daddr, dport] = time;
	ind[saddr, sport, daddr, dport] = sprintf("(%s,%s)->(%s,%s)", saddr, sport, daddr, dport);
}
if ((cwnd >= 100) && (!((saddr, sport, daddr, dport) in cwndtime))) {
	cwndtime[saddr, sport, daddr, dport] = time;
	cwndhiack[saddr, sport, daddr, dport] = hiack;
}

if ((time >= mid) && (!((saddr, sport, daddr, dport) in middletime))) {
	middletime[saddr, sport, daddr, dport] = time;
	middlehiack[saddr, sport, daddr, dport] = hiack;
}

endtime[saddr, sport, daddr, dport] = time;
highest_ack[saddr, sport, daddr, dport] = hiack;



if (!((saddr, sport, daddr, dport) in cwndfile)) {
	cwndfile[saddr, sport, daddr, dport] = sprintf("cwnd-%d,%d-%d,%d.out", saddr, sport, daddr, dport);
	printf "TitleText: (%d,%d)->(%d,%d)\n", saddr, sport, daddr, dport > cwndfile[saddr,sport,daddr,dport];
	printf "Device: Postscript\n" > cwndfile[saddr,sport,daddr,dport];
}
if (!((saddr, sport, daddr, dport) in ssthreshfile)) {
	ssthreshfile[saddr, sport, daddr, dport] = sprintf("ssthresh-%d,%d-%d,%d.out", saddr, sport, daddr, dport);
	printf "TitleText: (%d,%d)->(%d,%d)\n", saddr, sport, daddr, dport > ssthreshfile[saddr,sport,daddr,dport];
	printf "Device: Postscript\n" > ssthreshfile[saddr,sport,daddr,dport];
}
if (!((saddr, sport, daddr, dport) in srttfile)) {
	srttfile[saddr, sport, daddr, dport] = sprintf("srtt-%d,%d-%d,%d.out", saddr, sport, daddr, dport);
	printf "TitleText: (%d,%d)->(%d,%d)\n", saddr, sport, daddr, dport > srttfile[saddr,sport,daddr,dport];
	printf "Device: Postscript\n" > srttfile[saddr,sport,daddr,dport];
}
if (!((saddr, sport, daddr, dport) in rttvarfile)) {
	rttvarfile[saddr, sport, daddr, dport] = sprintf("rttvar-%d,%d-%d,%d.out", saddr, sport, daddr, dport);
	printf "TitleText: (%d,%d)->(%d,%d)\n", saddr, sport, daddr, dport > rttvarfile[saddr,sport,daddr,dport];
	printf "Device: Postscript\n" > rttvarfile[saddr,sport,daddr,dport];
}

printf "%g %g\n", time, cwnd > cwndfile[saddr, sport, daddr, dport];
printf "%g %d\n", time, ssthresh > ssthreshfile[saddr, sport, daddr, dport];
printf "%g %g\n", time, srtt > srttfile[saddr, sport, daddr, dport];
printf "%g %g\n", time, rttvar > rttvarfile[saddr, sport, daddr, dport];
1} 
END {
	for (f in cwndfile) {
		close(cwndfile[f]);
	}
	for (f in ssthreshfile) {
		close(ssthreshfile[f]);
	}
	for (f in srttfile) {
		close(srttfile[f]);
	}
	for (f in rttvarfile) {
		close(rttvarfile[f]);
	}
 	for (i in starttime) {
 		bw = (highest_ack[i]/(endtime[i] - starttime[i]))*8.0;
		duration = endtime[i] - starttime[i];
		if (i in cwndtime) {
			ss_bw = ((highest_ack[i] - cwndhiack[i])/(endtime[i] - cwndtime[i]))*8.0;
			ss_starttime = cwndtime[i] - starttime[i];
		}
		else {
			ss_bw = -1;
			ss_starttime = -1;
		}
		if (i in middletime) {
			sh_bw = ((highest_ack[i] - middlehiack[i])/(endtime[i] - middletime[i]))*8.0;
		}
		else {
			sh_bw = -1;
		}
 		print ind[i], duration, bw, ss_starttime, ss_bw, sh_bw > "thruput";
 	}
	close("thruput");
}
