#
# ns-emulate.tcl
#	configuration for ns emulation
#
if [TclObject is-class Network/Pcap/Live] {
	Network/Pcap/Live set snaplen_ 4096
	Network/Pcap/Live set promisc_ false
	Network/Pcap/Live set timeout_ 0
	Network/Pcap/Live set optimize_ true
}
