# Traffic file for the 100 node topology for diffusion

set src_(0) [new Application/DiffApp/PingSender]
$ns_ attach-diffapp $node_(0) $src_(0)
$ns_ at 0.123 "$src_(0) publish"

set src_(10) [new Application/DiffApp/PingSender]
$ns_ attach-diffapp $node_(10) $src_(10)
$ns_ at 1.123 "$src_(10) publish"

set src_(20) [new Application/DiffApp/PingSender]
$ns_ attach-diffapp $node_(20) $src_(20)
$ns_ at 2.512 "$src_(20) publish"

set src_(30) [new Application/DiffApp/PingSender]
$ns_ attach-diffapp $node_(30) $src_(30)
$ns_ at 3.644 "$src_(30) publish"

set src_(40) [new Application/DiffApp/PingSender]
$ns_ attach-diffapp $node_(40) $src_(40)
$ns_ at 4.134 "$src_(40) publish"

set src_(50) [new Application/DiffApp/PingSender]
$ns_ attach-diffapp $node_(50) $src_(50)
$ns_ at 5.289 "$src_(50) publish"

set src_(60) [new Application/DiffApp/PingSender]
$ns_ attach-diffapp $node_(60) $src_(60)
$ns_ at 3.512 "$src_(60) publish"

set src_(70) [new Application/DiffApp/PingSender]
$ns_ attach-diffapp $node_(70) $src_(70)
$ns_ at 3.640 "$src_(70) publish"

set src_(80) [new Application/DiffApp/PingSender]
$ns_ attach-diffapp $node_(80) $src_(80)
$ns_ at 3.134 "$src_(80) publish"

set src_(90) [new Application/DiffApp/PingSender]
$ns_ attach-diffapp $node_(90) $src_(90)
$ns_ at 2.289 "$src_(50) publish"

set src_(100) [new Application/DiffApp/PingSender]
$ns_ attach-diffapp $node_(100) $src_(100)
$ns_ at 2.923 "$src_(100) publish"

#Diffusion sink application
set snk_(95) [new Application/DiffApp/PingReceiver]
$ns_ attach-diffapp $node_(95) $snk_(95)
$ns_ at 1.329 "$snk_(95) subscribe"

#Diffusion sink application
set snk_(85) [new Application/DiffApp/PingReceiver]
$ns_ attach-diffapp $node_(85) $snk_(85)
$ns_ at 1.422 "$snk_(85) subscribe"

#Diffusion sink application
set snk_(75) [new Application/DiffApp/PingReceiver]
$ns_ attach-diffapp $node_(75) $snk_(75)
$ns_ at 2.691 "$snk_(75) subscribe"

#Diffusion sink application
set snk_(65) [new Application/DiffApp/PingReceiver]
$ns_ attach-diffapp $node_(65) $snk_(65)
$ns_ at 2.168 "$snk_(65) subscribe"

#Diffusion sink application
set snk_(55) [new Application/DiffApp/PingReceiver]
$ns_ attach-diffapp $node_(55) $snk_(55)
$ns_ at 3.692 "$snk_(55) subscribe"

