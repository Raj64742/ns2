#
# ns-emulate.tcl
#	defaults for nse [ns with emulation support]
#
if [TclObject is-class Network/Pcap/Live] {
	Network/Pcap/Live set snaplen_ 4096;# bpf snap len
	Network/Pcap/Live set promisc_ false;
	Network/Pcap/Live set timeout_ 0
	Network/Pcap/Live set optimize_ true;# bpf code optimizer
}

if [TclObject is-class Agent/Tap] {
	Agent/Tap set maxpkt_ 1600
}
