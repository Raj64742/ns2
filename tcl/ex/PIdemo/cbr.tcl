#
# nodes: 50, max conn: 50, send rate: 1, seed: 1
#
#
# 0 connecting to 1 at time 98.653341107374686
#
set udp_(0) [new Agent/UDP]
$ns_ attach-agent $node_(0) $udp_(0)
set null_(0) [new Agent/Null]
$ns_ attach-agent $node_(1) $null_(0)
set cbr_(0) [new Application/Traffic/CBR]
$cbr_(0) set packetSize_ 512
$cbr_(0) set interval_ 1
$cbr_(0) set random_ 1
$cbr_(0) set maxpkts_ 10000
$cbr_(0) attach-agent $udp_(0)
$ns_ connect $udp_(0) $null_(0)
$ns_ at 98.653341107374686 "$cbr_(0) start"
$ns_ at 848.65334110737467 "$cbr_(0) stop"
#
# 1 connecting to 25 at time 343.98759894258694
#
set udp_(1) [new Agent/UDP]
$ns_ attach-agent $node_(1) $udp_(1)
set null_(1) [new Agent/Null]
$ns_ attach-agent $node_(25) $null_(1)
set cbr_(1) [new Application/Traffic/CBR]
$cbr_(1) set packetSize_ 512
$cbr_(1) set interval_ 1
$cbr_(1) set random_ 1
$cbr_(1) set maxpkts_ 10000
$cbr_(1) attach-agent $udp_(1)
$ns_ connect $udp_(1) $null_(1)
$ns_ at 343.98759894258694 "$cbr_(1) start"
$ns_ at 1093.9875989425868 "$cbr_(1) stop"
#
# 2 connecting to 3 at time 164.21938974606778
#
set udp_(2) [new Agent/UDP]
$ns_ attach-agent $node_(2) $udp_(2)
set null_(2) [new Agent/Null]
$ns_ attach-agent $node_(3) $null_(2)
set cbr_(2) [new Application/Traffic/CBR]
$cbr_(2) set packetSize_ 512
$cbr_(2) set interval_ 1
$cbr_(2) set random_ 1
$cbr_(2) set maxpkts_ 10000
$cbr_(2) attach-agent $udp_(2)
$ns_ connect $udp_(2) $null_(2)
$ns_ at 164.21938974606778 "$cbr_(2) start"
$ns_ at 914.2193897460678 "$cbr_(2) stop"
#
# 3 connecting to 5 at time 509.14853765123922
#
set udp_(3) [new Agent/UDP]
$ns_ attach-agent $node_(3) $udp_(3)
set null_(3) [new Agent/Null]
$ns_ attach-agent $node_(5) $null_(3)
set cbr_(3) [new Application/Traffic/CBR]
$cbr_(3) set packetSize_ 512
$cbr_(3) set interval_ 1
$cbr_(3) set random_ 1
$cbr_(3) set maxpkts_ 10000
$cbr_(3) attach-agent $udp_(3)
$ns_ connect $udp_(3) $null_(3)
$ns_ at 509.14853765123922 "$cbr_(3) start"
$ns_ at 1259.1485376512392 "$cbr_(3) stop"
#
# 4 connecting to 17 at time 701.01967195562065
#
set udp_(4) [new Agent/UDP]
$ns_ attach-agent $node_(4) $udp_(4)
set null_(4) [new Agent/Null]
$ns_ attach-agent $node_(17) $null_(4)
set cbr_(4) [new Application/Traffic/CBR]
$cbr_(4) set packetSize_ 512
$cbr_(4) set interval_ 1
$cbr_(4) set random_ 1
$cbr_(4) set maxpkts_ 10000
$cbr_(4) attach-agent $udp_(4)
$ns_ connect $udp_(4) $null_(4)
$ns_ at 701.01967195562065 "$cbr_(4) start"
$ns_ at 1451.0196719556207 "$cbr_(4) stop"
#
# 5 connecting to 39 at time 389.56227905096591
#
set udp_(5) [new Agent/UDP]
$ns_ attach-agent $node_(5) $udp_(5)
set null_(5) [new Agent/Null]
$ns_ attach-agent $node_(39) $null_(5)
set cbr_(5) [new Application/Traffic/CBR]
$cbr_(5) set packetSize_ 512
$cbr_(5) set interval_ 1
$cbr_(5) set random_ 1
$cbr_(5) set maxpkts_ 10000
$cbr_(5) attach-agent $udp_(5)
$ns_ connect $udp_(5) $null_(5)
$ns_ at 389.56227905096591 "$cbr_(5) start"
$ns_ at 1139.562279050966 "$cbr_(5) stop"
#
# 6 connecting to 33 at time 25.929082895596085
#
set udp_(6) [new Agent/UDP]
$ns_ attach-agent $node_(6) $udp_(6)
set null_(6) [new Agent/Null]
$ns_ attach-agent $node_(33) $null_(6)
set cbr_(6) [new Application/Traffic/CBR]
$cbr_(6) set packetSize_ 512
$cbr_(6) set interval_ 1
$cbr_(6) set random_ 1
$cbr_(6) set maxpkts_ 10000
$cbr_(6) attach-agent $udp_(6)
$ns_ connect $udp_(6) $null_(6)
$ns_ at 25.929082895596085 "$cbr_(6) start"
$ns_ at 775.92908289559614 "$cbr_(6) stop"
#
# 7 connecting to 6 at time 397.27514500137198
#
set udp_(7) [new Agent/UDP]
$ns_ attach-agent $node_(7) $udp_(7)
set null_(7) [new Agent/Null]
$ns_ attach-agent $node_(6) $null_(7)
set cbr_(7) [new Application/Traffic/CBR]
$cbr_(7) set packetSize_ 512
$cbr_(7) set interval_ 1
$cbr_(7) set random_ 1
$cbr_(7) set maxpkts_ 10000
$cbr_(7) attach-agent $udp_(7)
$ns_ connect $udp_(7) $null_(7)
$ns_ at 397.27514500137198 "$cbr_(7) start"
$ns_ at 1147.275145001372 "$cbr_(7) stop"
#
# 8 connecting to 17 at time 5.7736396583605742
#
set udp_(8) [new Agent/UDP]
$ns_ attach-agent $node_(8) $udp_(8)
set null_(8) [new Agent/Null]
$ns_ attach-agent $node_(17) $null_(8)
set cbr_(8) [new Application/Traffic/CBR]
$cbr_(8) set packetSize_ 512
$cbr_(8) set interval_ 1
$cbr_(8) set random_ 1
$cbr_(8) set maxpkts_ 10000
$cbr_(8) attach-agent $udp_(8)
$ns_ connect $udp_(8) $null_(8)
$ns_ at 5.7736396583605742 "$cbr_(8) start"
$ns_ at 755.77363965836059 "$cbr_(8) stop"
#
# 9 connecting to 39 at time 50.131678138920883
#
set udp_(9) [new Agent/UDP]
$ns_ attach-agent $node_(9) $udp_(9)
set null_(9) [new Agent/Null]
$ns_ attach-agent $node_(39) $null_(9)
set cbr_(9) [new Application/Traffic/CBR]
$cbr_(9) set packetSize_ 512
$cbr_(9) set interval_ 1
$cbr_(9) set random_ 1
$cbr_(9) set maxpkts_ 10000
$cbr_(9) attach-agent $udp_(9)
$ns_ connect $udp_(9) $null_(9)
$ns_ at 50.131678138920883 "$cbr_(9) start"
$ns_ at 800.13167813892085 "$cbr_(9) stop"
#
# 10 connecting to 42 at time 515.07953427037205
#
set udp_(10) [new Agent/UDP]
$ns_ attach-agent $node_(10) $udp_(10)
set null_(10) [new Agent/Null]
$ns_ attach-agent $node_(42) $null_(10)
set cbr_(10) [new Application/Traffic/CBR]
$cbr_(10) set packetSize_ 512
$cbr_(10) set interval_ 1
$cbr_(10) set random_ 1
$cbr_(10) set maxpkts_ 10000
$cbr_(10) attach-agent $udp_(10)
$ns_ connect $udp_(10) $null_(10)
$ns_ at 515.07953427037205 "$cbr_(10) start"
$ns_ at 1265.0795342703721 "$cbr_(10) stop"
#
# 11 connecting to 8 at time 697.8273710458667
#
set udp_(11) [new Agent/UDP]
$ns_ attach-agent $node_(11) $udp_(11)
set null_(11) [new Agent/Null]
$ns_ attach-agent $node_(8) $null_(11)
set cbr_(11) [new Application/Traffic/CBR]
$cbr_(11) set packetSize_ 512
$cbr_(11) set interval_ 1
$cbr_(11) set random_ 1
$cbr_(11) set maxpkts_ 10000
$cbr_(11) attach-agent $udp_(11)
$ns_ connect $udp_(11) $null_(11)
$ns_ at 697.8273710458667 "$cbr_(11) start"
$ns_ at 1447.8273710458666 "$cbr_(11) stop"
#
# 12 connecting to 34 at time 395.1965831896274
#
set udp_(12) [new Agent/UDP]
$ns_ attach-agent $node_(12) $udp_(12)
set null_(12) [new Agent/Null]
$ns_ attach-agent $node_(34) $null_(12)
set cbr_(12) [new Application/Traffic/CBR]
$cbr_(12) set packetSize_ 512
$cbr_(12) set interval_ 1
$cbr_(12) set random_ 1
$cbr_(12) set maxpkts_ 10000
$cbr_(12) attach-agent $udp_(12)
$ns_ connect $udp_(12) $null_(12)
$ns_ at 395.1965831896274 "$cbr_(12) start"
$ns_ at 1145.1965831896273 "$cbr_(12) stop"
#
# 13 connecting to 10 at time 490.43922172414102
#
set udp_(13) [new Agent/UDP]
$ns_ attach-agent $node_(13) $udp_(13)
set null_(13) [new Agent/Null]
$ns_ attach-agent $node_(10) $null_(13)
set cbr_(13) [new Application/Traffic/CBR]
$cbr_(13) set packetSize_ 512
$cbr_(13) set interval_ 1
$cbr_(13) set random_ 1
$cbr_(13) set maxpkts_ 10000
$cbr_(13) attach-agent $udp_(13)
$ns_ connect $udp_(13) $null_(13)
$ns_ at 490.43922172414102 "$cbr_(13) start"
$ns_ at 1240.4392217241411 "$cbr_(13) stop"
#
# 14 connecting to 42 at time 525.89294583345441
#
set udp_(14) [new Agent/UDP]
$ns_ attach-agent $node_(14) $udp_(14)
set null_(14) [new Agent/Null]
$ns_ attach-agent $node_(42) $null_(14)
set cbr_(14) [new Application/Traffic/CBR]
$cbr_(14) set packetSize_ 512
$cbr_(14) set interval_ 1
$cbr_(14) set random_ 1
$cbr_(14) set maxpkts_ 10000
$cbr_(14) attach-agent $udp_(14)
$ns_ connect $udp_(14) $null_(14)
$ns_ at 525.89294583345441 "$cbr_(14) start"
$ns_ at 1275.8929458334544 "$cbr_(14) stop"
#
# 15 connecting to 41 at time 571.64852976410111
#
set udp_(15) [new Agent/UDP]
$ns_ attach-agent $node_(15) $udp_(15)
set null_(15) [new Agent/Null]
$ns_ attach-agent $node_(41) $null_(15)
set cbr_(15) [new Application/Traffic/CBR]
$cbr_(15) set packetSize_ 512
$cbr_(15) set interval_ 1
$cbr_(15) set random_ 1
$cbr_(15) set maxpkts_ 10000
$cbr_(15) attach-agent $udp_(15)
$ns_ connect $udp_(15) $null_(15)
$ns_ at 571.64852976410111 "$cbr_(15) start"
$ns_ at 1321.6485297641011 "$cbr_(15) stop"
#
# 16 connecting to 27 at time 35.598385280742491
#
set udp_(16) [new Agent/UDP]
$ns_ attach-agent $node_(16) $udp_(16)
set null_(16) [new Agent/Null]
$ns_ attach-agent $node_(27) $null_(16)
set cbr_(16) [new Application/Traffic/CBR]
$cbr_(16) set packetSize_ 512
$cbr_(16) set interval_ 1
$cbr_(16) set random_ 1
$cbr_(16) set maxpkts_ 10000
$cbr_(16) attach-agent $udp_(16)
$ns_ connect $udp_(16) $null_(16)
$ns_ at 35.598385280742491 "$cbr_(16) start"
$ns_ at 785.59838528074249 "$cbr_(16) stop"
#
# 17 connecting to 23 at time 246.17566959754362
#
set udp_(17) [new Agent/UDP]
$ns_ attach-agent $node_(17) $udp_(17)
set null_(17) [new Agent/Null]
$ns_ attach-agent $node_(23) $null_(17)
set cbr_(17) [new Application/Traffic/CBR]
$cbr_(17) set packetSize_ 512
$cbr_(17) set interval_ 1
$cbr_(17) set random_ 1
$cbr_(17) set maxpkts_ 10000
$cbr_(17) attach-agent $udp_(17)
$ns_ connect $udp_(17) $null_(17)
$ns_ at 246.17566959754362 "$cbr_(17) start"
$ns_ at 996.17566959754367 "$cbr_(17) stop"
#
# 18 connecting to 13 at time 567.30786446356581
#
set udp_(18) [new Agent/UDP]
$ns_ attach-agent $node_(18) $udp_(18)
set null_(18) [new Agent/Null]
$ns_ attach-agent $node_(13) $null_(18)
set cbr_(18) [new Application/Traffic/CBR]
$cbr_(18) set packetSize_ 512
$cbr_(18) set interval_ 1
$cbr_(18) set random_ 1
$cbr_(18) set maxpkts_ 10000
$cbr_(18) attach-agent $udp_(18)
$ns_ connect $udp_(18) $null_(18)
$ns_ at 567.30786446356581 "$cbr_(18) start"
$ns_ at 1317.3078644635657 "$cbr_(18) stop"
#
# 19 connecting to 49 at time 274.0040030442197
#
set udp_(19) [new Agent/UDP]
$ns_ attach-agent $node_(19) $udp_(19)
set null_(19) [new Agent/Null]
$ns_ attach-agent $node_(49) $null_(19)
set cbr_(19) [new Application/Traffic/CBR]
$cbr_(19) set packetSize_ 512
$cbr_(19) set interval_ 1
$cbr_(19) set random_ 1
$cbr_(19) set maxpkts_ 10000
$cbr_(19) attach-agent $udp_(19)
$ns_ connect $udp_(19) $null_(19)
$ns_ at 274.0040030442197 "$cbr_(19) start"
$ns_ at 1024.0040030442196 "$cbr_(19) stop"
#
# 20 connecting to 25 at time 736.91271466059277
#
set udp_(20) [new Agent/UDP]
$ns_ attach-agent $node_(20) $udp_(20)
set null_(20) [new Agent/Null]
$ns_ attach-agent $node_(25) $null_(20)
set cbr_(20) [new Application/Traffic/CBR]
$cbr_(20) set packetSize_ 512
$cbr_(20) set interval_ 1
$cbr_(20) set random_ 1
$cbr_(20) set maxpkts_ 10000
$cbr_(20) attach-agent $udp_(20)
$ns_ connect $udp_(20) $null_(20)
$ns_ at 736.91271466059277 "$cbr_(20) start"
$ns_ at 1486.9127146605929 "$cbr_(20) stop"
#
# 21 connecting to 22 at time 565.01687623794044
#
set udp_(21) [new Agent/UDP]
$ns_ attach-agent $node_(21) $udp_(21)
set null_(21) [new Agent/Null]
$ns_ attach-agent $node_(22) $null_(21)
set cbr_(21) [new Application/Traffic/CBR]
$cbr_(21) set packetSize_ 512
$cbr_(21) set interval_ 1
$cbr_(21) set random_ 1
$cbr_(21) set maxpkts_ 10000
$cbr_(21) attach-agent $udp_(21)
$ns_ connect $udp_(21) $null_(21)
$ns_ at 565.01687623794044 "$cbr_(21) start"
$ns_ at 1315.0168762379403 "$cbr_(21) stop"
#
# 22 connecting to 15 at time 54.514412211493784
#
set udp_(22) [new Agent/UDP]
$ns_ attach-agent $node_(22) $udp_(22)
set null_(22) [new Agent/Null]
$ns_ attach-agent $node_(15) $null_(22)
set cbr_(22) [new Application/Traffic/CBR]
$cbr_(22) set packetSize_ 512
$cbr_(22) set interval_ 1
$cbr_(22) set random_ 1
$cbr_(22) set maxpkts_ 10000
$cbr_(22) attach-agent $udp_(22)
$ns_ connect $udp_(22) $null_(22)
$ns_ at 54.514412211493784 "$cbr_(22) start"
$ns_ at 804.51441221149378 "$cbr_(22) stop"
#
# 23 connecting to 13 at time 663.53034643155081
#
set udp_(23) [new Agent/UDP]
$ns_ attach-agent $node_(23) $udp_(23)
set null_(23) [new Agent/Null]
$ns_ attach-agent $node_(13) $null_(23)
set cbr_(23) [new Application/Traffic/CBR]
$cbr_(23) set packetSize_ 512
$cbr_(23) set interval_ 1
$cbr_(23) set random_ 1
$cbr_(23) set maxpkts_ 10000
$cbr_(23) attach-agent $udp_(23)
$ns_ connect $udp_(23) $null_(23)
$ns_ at 663.53034643155081 "$cbr_(23) start"
$ns_ at 1413.5303464315507 "$cbr_(23) stop"
#
# 24 connecting to 28 at time 327.30855423831781
#
set udp_(24) [new Agent/UDP]
$ns_ attach-agent $node_(24) $udp_(24)
set null_(24) [new Agent/Null]
$ns_ attach-agent $node_(28) $null_(24)
set cbr_(24) [new Application/Traffic/CBR]
$cbr_(24) set packetSize_ 512
$cbr_(24) set interval_ 1
$cbr_(24) set random_ 1
$cbr_(24) set maxpkts_ 10000
$cbr_(24) attach-agent $udp_(24)
$ns_ connect $udp_(24) $null_(24)
$ns_ at 327.30855423831781 "$cbr_(24) start"
$ns_ at 1077.3085542383178 "$cbr_(24) stop"
#
# 25 connecting to 26 at time 358.29882375351099
#
set udp_(25) [new Agent/UDP]
$ns_ attach-agent $node_(25) $udp_(25)
set null_(25) [new Agent/Null]
$ns_ attach-agent $node_(26) $null_(25)
set cbr_(25) [new Application/Traffic/CBR]
$cbr_(25) set packetSize_ 512
$cbr_(25) set interval_ 1
$cbr_(25) set random_ 1
$cbr_(25) set maxpkts_ 10000
$cbr_(25) attach-agent $udp_(25)
$ns_ connect $udp_(25) $null_(25)
$ns_ at 358.29882375351099 "$cbr_(25) start"
$ns_ at 1108.298823753511 "$cbr_(25) stop"
#
# 26 connecting to 24 at time 206.18013022755281
#
set udp_(26) [new Agent/UDP]
$ns_ attach-agent $node_(26) $udp_(26)
set null_(26) [new Agent/Null]
$ns_ attach-agent $node_(24) $null_(26)
set cbr_(26) [new Application/Traffic/CBR]
$cbr_(26) set packetSize_ 512
$cbr_(26) set interval_ 1
$cbr_(26) set random_ 1
$cbr_(26) set maxpkts_ 10000
$cbr_(26) attach-agent $udp_(26)
$ns_ connect $udp_(26) $null_(26)
$ns_ at 206.18013022755281 "$cbr_(26) start"
$ns_ at 956.18013022755281 "$cbr_(26) stop"
#
# 27 connecting to 36 at time 124.88040031161178
#
set udp_(27) [new Agent/UDP]
$ns_ attach-agent $node_(27) $udp_(27)
set null_(27) [new Agent/Null]
$ns_ attach-agent $node_(36) $null_(27)
set cbr_(27) [new Application/Traffic/CBR]
$cbr_(27) set packetSize_ 512
$cbr_(27) set interval_ 1
$cbr_(27) set random_ 1
$cbr_(27) set maxpkts_ 10000
$cbr_(27) attach-agent $udp_(27)
$ns_ connect $udp_(27) $null_(27)
$ns_ at 124.88040031161178 "$cbr_(27) start"
$ns_ at 874.88040031161177 "$cbr_(27) stop"
#
# 28 connecting to 49 at time 673.24221491498975
#
set udp_(28) [new Agent/UDP]
$ns_ attach-agent $node_(28) $udp_(28)
set null_(28) [new Agent/Null]
$ns_ attach-agent $node_(49) $null_(28)
set cbr_(28) [new Application/Traffic/CBR]
$cbr_(28) set packetSize_ 512
$cbr_(28) set interval_ 1
$cbr_(28) set random_ 1
$cbr_(28) set maxpkts_ 10000
$cbr_(28) attach-agent $udp_(28)
$ns_ connect $udp_(28) $null_(28)
$ns_ at 673.24221491498975 "$cbr_(28) start"
$ns_ at 1423.2422149149897 "$cbr_(28) stop"
#
# 29 connecting to 40 at time 45.423245660692103
#
set udp_(29) [new Agent/UDP]
$ns_ attach-agent $node_(29) $udp_(29)
set null_(29) [new Agent/Null]
$ns_ attach-agent $node_(40) $null_(29)
set cbr_(29) [new Application/Traffic/CBR]
$cbr_(29) set packetSize_ 512
$cbr_(29) set interval_ 1
$cbr_(29) set random_ 1
$cbr_(29) set maxpkts_ 10000
$cbr_(29) attach-agent $udp_(29)
$ns_ connect $udp_(29) $null_(29)
$ns_ at 45.423245660692103 "$cbr_(29) start"
$ns_ at 795.42324566069215 "$cbr_(29) stop"
#
# 30 connecting to 40 at time 378.39217105805506
#
set udp_(30) [new Agent/UDP]
$ns_ attach-agent $node_(30) $udp_(30)
set null_(30) [new Agent/Null]
$ns_ attach-agent $node_(40) $null_(30)
set cbr_(30) [new Application/Traffic/CBR]
$cbr_(30) set packetSize_ 512
$cbr_(30) set interval_ 1
$cbr_(30) set random_ 1
$cbr_(30) set maxpkts_ 10000
$cbr_(30) attach-agent $udp_(30)
$ns_ connect $udp_(30) $null_(30)
$ns_ at 378.39217105805506 "$cbr_(30) start"
$ns_ at 1128.3921710580551 "$cbr_(30) stop"
#
# 31 connecting to 1 at time 239.2747058716019
#
set udp_(31) [new Agent/UDP]
$ns_ attach-agent $node_(31) $udp_(31)
set null_(31) [new Agent/Null]
$ns_ attach-agent $node_(1) $null_(31)
set cbr_(31) [new Application/Traffic/CBR]
$cbr_(31) set packetSize_ 512
$cbr_(31) set interval_ 1
$cbr_(31) set random_ 1
$cbr_(31) set maxpkts_ 10000
$cbr_(31) attach-agent $udp_(31)
$ns_ connect $udp_(31) $null_(31)
$ns_ at 239.2747058716019 "$cbr_(31) start"
$ns_ at 989.2747058716019 "$cbr_(31) stop"
#
# 32 connecting to 48 at time 370.48251373715817
#
set udp_(32) [new Agent/UDP]
$ns_ attach-agent $node_(32) $udp_(32)
set null_(32) [new Agent/Null]
$ns_ attach-agent $node_(48) $null_(32)
set cbr_(32) [new Application/Traffic/CBR]
$cbr_(32) set packetSize_ 512
$cbr_(32) set interval_ 1
$cbr_(32) set random_ 1
$cbr_(32) set maxpkts_ 10000
$cbr_(32) attach-agent $udp_(32)
$ns_ connect $udp_(32) $null_(32)
$ns_ at 370.48251373715817 "$cbr_(32) start"
$ns_ at 1120.4825137371581 "$cbr_(32) stop"
#
# 33 connecting to 27 at time 68.049671160080322
#
set udp_(33) [new Agent/UDP]
$ns_ attach-agent $node_(33) $udp_(33)
set null_(33) [new Agent/Null]
$ns_ attach-agent $node_(27) $null_(33)
set cbr_(33) [new Application/Traffic/CBR]
$cbr_(33) set packetSize_ 512
$cbr_(33) set interval_ 1
$cbr_(33) set random_ 1
$cbr_(33) set maxpkts_ 10000
$cbr_(33) attach-agent $udp_(33)
$ns_ connect $udp_(33) $null_(33)
$ns_ at 68.049671160080322 "$cbr_(33) start"
$ns_ at 818.04967116008038 "$cbr_(33) stop"
#
# 34 connecting to 44 at time 55.31180640929928
#
set udp_(34) [new Agent/UDP]
$ns_ attach-agent $node_(34) $udp_(34)
set null_(34) [new Agent/Null]
$ns_ attach-agent $node_(44) $null_(34)
set cbr_(34) [new Application/Traffic/CBR]
$cbr_(34) set packetSize_ 512
$cbr_(34) set interval_ 1
$cbr_(34) set random_ 1
$cbr_(34) set maxpkts_ 10000
$cbr_(34) attach-agent $udp_(34)
$ns_ connect $udp_(34) $null_(34)
$ns_ at 55.31180640929928 "$cbr_(34) start"
$ns_ at 805.31180640929927 "$cbr_(34) stop"
#
# 35 connecting to 0 at time 288.10661101625607
#
set udp_(35) [new Agent/UDP]
$ns_ attach-agent $node_(35) $udp_(35)
set null_(35) [new Agent/Null]
$ns_ attach-agent $node_(0) $null_(35)
set cbr_(35) [new Application/Traffic/CBR]
$cbr_(35) set packetSize_ 512
$cbr_(35) set interval_ 1
$cbr_(35) set random_ 1
$cbr_(35) set maxpkts_ 10000
$cbr_(35) attach-agent $udp_(35)
$ns_ connect $udp_(35) $null_(35)
$ns_ at 288.10661101625607 "$cbr_(35) start"
$ns_ at 1038.106611016256 "$cbr_(35) stop"
#
# 36 connecting to 28 at time 685.36308148659907
#
set udp_(36) [new Agent/UDP]
$ns_ attach-agent $node_(36) $udp_(36)
set null_(36) [new Agent/Null]
$ns_ attach-agent $node_(28) $null_(36)
set cbr_(36) [new Application/Traffic/CBR]
$cbr_(36) set packetSize_ 512
$cbr_(36) set interval_ 1
$cbr_(36) set random_ 1
$cbr_(36) set maxpkts_ 10000
$cbr_(36) attach-agent $udp_(36)
$ns_ connect $udp_(36) $null_(36)
$ns_ at 685.36308148659907 "$cbr_(36) start"
$ns_ at 1435.3630814865992 "$cbr_(36) stop"
#
# 37 connecting to 2 at time 348.33436871335579
#
set udp_(37) [new Agent/UDP]
$ns_ attach-agent $node_(37) $udp_(37)
set null_(37) [new Agent/Null]
$ns_ attach-agent $node_(2) $null_(37)
set cbr_(37) [new Application/Traffic/CBR]
$cbr_(37) set packetSize_ 512
$cbr_(37) set interval_ 1
$cbr_(37) set random_ 1
$cbr_(37) set maxpkts_ 10000
$cbr_(37) attach-agent $udp_(37)
$ns_ connect $udp_(37) $null_(37)
$ns_ at 348.33436871335579 "$cbr_(37) start"
$ns_ at 1098.3343687133558 "$cbr_(37) stop"
#
# 38 connecting to 44 at time 37.562987784651568
#
set udp_(38) [new Agent/UDP]
$ns_ attach-agent $node_(38) $udp_(38)
set null_(38) [new Agent/Null]
$ns_ attach-agent $node_(44) $null_(38)
set cbr_(38) [new Application/Traffic/CBR]
$cbr_(38) set packetSize_ 512
$cbr_(38) set interval_ 1
$cbr_(38) set random_ 1
$cbr_(38) set maxpkts_ 10000
$cbr_(38) attach-agent $udp_(38)
$ns_ connect $udp_(38) $null_(38)
$ns_ at 37.562987784651568 "$cbr_(38) start"
$ns_ at 787.56298778465157 "$cbr_(38) stop"
#
# 39 connecting to 26 at time 577.65341006575773
#
set udp_(39) [new Agent/UDP]
$ns_ attach-agent $node_(39) $udp_(39)
set null_(39) [new Agent/Null]
$ns_ attach-agent $node_(26) $null_(39)
set cbr_(39) [new Application/Traffic/CBR]
$cbr_(39) set packetSize_ 512
$cbr_(39) set interval_ 1
$cbr_(39) set random_ 1
$cbr_(39) set maxpkts_ 10000
$cbr_(39) attach-agent $udp_(39)
$ns_ connect $udp_(39) $null_(39)
$ns_ at 577.65341006575773 "$cbr_(39) start"
$ns_ at 1327.6534100657577 "$cbr_(39) stop"
#
# 40 connecting to 32 at time 94.024031699646272
#
set udp_(40) [new Agent/UDP]
$ns_ attach-agent $node_(40) $udp_(40)
set null_(40) [new Agent/Null]
$ns_ attach-agent $node_(32) $null_(40)
set cbr_(40) [new Application/Traffic/CBR]
$cbr_(40) set packetSize_ 512
$cbr_(40) set interval_ 1
$cbr_(40) set random_ 1
$cbr_(40) set maxpkts_ 10000
$cbr_(40) attach-agent $udp_(40)
$ns_ connect $udp_(40) $null_(40)
$ns_ at 94.024031699646272 "$cbr_(40) start"
$ns_ at 844.02403169964623 "$cbr_(40) stop"
#
# 41 connecting to 2 at time 516.34147589390238
#
set udp_(41) [new Agent/UDP]
$ns_ attach-agent $node_(41) $udp_(41)
set null_(41) [new Agent/Null]
$ns_ attach-agent $node_(2) $null_(41)
set cbr_(41) [new Application/Traffic/CBR]
$cbr_(41) set packetSize_ 512
$cbr_(41) set interval_ 1
$cbr_(41) set random_ 1
$cbr_(41) set maxpkts_ 10000
$cbr_(41) attach-agent $udp_(41)
$ns_ connect $udp_(41) $null_(41)
$ns_ at 516.34147589390238 "$cbr_(41) start"
$ns_ at 1266.3414758939025 "$cbr_(41) stop"
#
# 42 connecting to 36 at time 472.1575634191546
#
set udp_(42) [new Agent/UDP]
$ns_ attach-agent $node_(42) $udp_(42)
set null_(42) [new Agent/Null]
$ns_ attach-agent $node_(36) $null_(42)
set cbr_(42) [new Application/Traffic/CBR]
$cbr_(42) set packetSize_ 512
$cbr_(42) set interval_ 1
$cbr_(42) set random_ 1
$cbr_(42) set maxpkts_ 10000
$cbr_(42) attach-agent $udp_(42)
$ns_ connect $udp_(42) $null_(42)
$ns_ at 472.1575634191546 "$cbr_(42) start"
$ns_ at 1222.1575634191545 "$cbr_(42) stop"
#
# 43 connecting to 23 at time 544.05899883436916
#
set udp_(43) [new Agent/UDP]
$ns_ attach-agent $node_(43) $udp_(43)
set null_(43) [new Agent/Null]
$ns_ attach-agent $node_(23) $null_(43)
set cbr_(43) [new Application/Traffic/CBR]
$cbr_(43) set packetSize_ 512
$cbr_(43) set interval_ 1
$cbr_(43) set random_ 1
$cbr_(43) set maxpkts_ 10000
$cbr_(43) attach-agent $udp_(43)
$ns_ connect $udp_(43) $null_(43)
$ns_ at 544.05899883436916 "$cbr_(43) start"
$ns_ at 1294.058998834369 "$cbr_(43) stop"
#
# 44 connecting to 49 at time 666.42916024030615
#
set udp_(44) [new Agent/UDP]
$ns_ attach-agent $node_(44) $udp_(44)
set null_(44) [new Agent/Null]
$ns_ attach-agent $node_(49) $null_(44)
set cbr_(44) [new Application/Traffic/CBR]
$cbr_(44) set packetSize_ 512
$cbr_(44) set interval_ 1
$cbr_(44) set random_ 1
$cbr_(44) set maxpkts_ 10000
$cbr_(44) attach-agent $udp_(44)
$ns_ connect $udp_(44) $null_(44)
$ns_ at 666.42916024030615 "$cbr_(44) start"
$ns_ at 1416.4291602403061 "$cbr_(44) stop"
#
# 45 connecting to 24 at time 229.74137332278369
#
set udp_(45) [new Agent/UDP]
$ns_ attach-agent $node_(45) $udp_(45)
set null_(45) [new Agent/Null]
$ns_ attach-agent $node_(24) $null_(45)
set cbr_(45) [new Application/Traffic/CBR]
$cbr_(45) set packetSize_ 512
$cbr_(45) set interval_ 1
$cbr_(45) set random_ 1
$cbr_(45) set maxpkts_ 10000
$cbr_(45) attach-agent $udp_(45)
$ns_ connect $udp_(45) $null_(45)
$ns_ at 229.74137332278369 "$cbr_(45) start"
$ns_ at 979.74137332278372 "$cbr_(45) stop"
#
# 46 connecting to 36 at time 384.95527668155512
#
set udp_(46) [new Agent/UDP]
$ns_ attach-agent $node_(46) $udp_(46)
set null_(46) [new Agent/Null]
$ns_ attach-agent $node_(36) $null_(46)
set cbr_(46) [new Application/Traffic/CBR]
$cbr_(46) set packetSize_ 512
$cbr_(46) set interval_ 1
$cbr_(46) set random_ 1
$cbr_(46) set maxpkts_ 10000
$cbr_(46) attach-agent $udp_(46)
$ns_ connect $udp_(46) $null_(46)
$ns_ at 384.95527668155512 "$cbr_(46) start"
$ns_ at 1134.955276681555 "$cbr_(46) stop"
#
# 47 connecting to 9 at time 634.48617008257941
#
set udp_(47) [new Agent/UDP]
$ns_ attach-agent $node_(47) $udp_(47)
set null_(47) [new Agent/Null]
$ns_ attach-agent $node_(9) $null_(47)
set cbr_(47) [new Application/Traffic/CBR]
$cbr_(47) set packetSize_ 512
$cbr_(47) set interval_ 1
$cbr_(47) set random_ 1
$cbr_(47) set maxpkts_ 10000
$cbr_(47) attach-agent $udp_(47)
$ns_ connect $udp_(47) $null_(47)
$ns_ at 634.48617008257941 "$cbr_(47) start"
$ns_ at 1384.4861700825795 "$cbr_(47) stop"
#
# 48 connecting to 42 at time 631.13297958911062
#
set udp_(48) [new Agent/UDP]
$ns_ attach-agent $node_(48) $udp_(48)
set null_(48) [new Agent/Null]
$ns_ attach-agent $node_(42) $null_(48)
set cbr_(48) [new Application/Traffic/CBR]
$cbr_(48) set packetSize_ 512
$cbr_(48) set interval_ 1
$cbr_(48) set random_ 1
$cbr_(48) set maxpkts_ 10000
$cbr_(48) attach-agent $udp_(48)
$ns_ connect $udp_(48) $null_(48)
$ns_ at 631.13297958911062 "$cbr_(48) start"
$ns_ at 1381.1329795891106 "$cbr_(48) stop"
#
# 49 connecting to 27 at time 311.545961681542
#
set udp_(49) [new Agent/UDP]
$ns_ attach-agent $node_(49) $udp_(49)
set null_(49) [new Agent/Null]
$ns_ attach-agent $node_(27) $null_(49)
set cbr_(49) [new Application/Traffic/CBR]
$cbr_(49) set packetSize_ 512
$cbr_(49) set interval_ 1
$cbr_(49) set random_ 1
$cbr_(49) set maxpkts_ 10000
$cbr_(49) attach-agent $udp_(49)
$ns_ connect $udp_(49) $null_(49)
$ns_ at 311.545961681542 "$cbr_(49) start"
$ns_ at 1061.5459616815419 "$cbr_(49) stop"
#
#Total sources/connections: 50/50
#
