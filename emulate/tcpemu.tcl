# Testing TCP emulation in ns 
# Author : Alefiya Hussain  and Ankur Sheth
# Date   : 05/14/2001
# 
# The transducer is constructed as follows :
#    Entry Node            Exit Node  
#        |                     |
#    Agent/TCPTap          Agent/TCPTap
#    Network/Pcap/Live     Network/IP
#
#  The TCPTap agents are attached to the nodes .
#
#  The transducer is the attached to a ns BayFullTcp agent. 
#
#
#                          node f - Agent/TCP/BayFullTcp (tcp)
#                          / \ 
#        ------------------   ---------------------
#       /                                          \
#      /                                            \
#   node 1 - Agent/TCPTap (tap1)                  node 2 - Agent/TCPTap (tap3)
#    (n1)    Network/Pcap/Live (bpf)                       Network/IP (ipnet)
#
#

set ns [new Simulator]
$ns use-scheduler RealTime

set f [open out.tr w]
$ns trace-all $f 

set entry_node [$ns node]
set exit_node [$ns node]
set tcp_node [$ns node]

$ns simplex-link $entry_node $tcp_node 10Mb 1ms DropTail  
$ns simplex-link $tcp_node $exit_node 10M 1ms DropTail

# Configure the entry node  
set tap1 [new Agent/TCPTap];               # Create the TCPTap Agent
set bpf [new Network/Pcap/Live];           # Create the bpf
set dev [$bpf open readonly xl0]
$bpf filter "src a.b.c.d and src port x and dst dummy_IP and dst port y"
$tap1 network $bpf;                        # Connect bpf to TCPTap Agent
$ns attach-agent $entry_node $tap1;        # Attach TCPTap Agent to the node

# Configure the exit node 
set tap2 [new Agent/TCPTap];       # Create a TCPTap Agent
set ipnet [new Network/IP];        # Create a Network agent
$ipnet open writeonly        
$tap2 network $ipnet;              # Connect network agent to tap agent
$tap2 advertised-window 512
$tap2 extipaddr "a.b.c.d"
$tap2 extport x
$ns attach-agent $exit_node $tap2;        # Attach agent to the node.

# Configure the TCP agent  
# Connect the agents.
$ns simplex-connect $tap1 $tcp
$ns simplex-connect $tcp $tap2


$ns at 0.01 "$tcp advance 1"
$ns at 20.0 "finish"

proc finish {} {
    global ns f nf
    $ns flush-trace
    close $f
    exit 0
}

$ns run




