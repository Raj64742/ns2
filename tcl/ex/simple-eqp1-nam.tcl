#
# Copyright (C) 1997 by USC/ISI
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 

#
# Maintainer: Kannan Varadhan <kannan@isi.edu>
#

proc nam_config {net} {
    $net node 0 circle
    $net node 1 circle
    $net node 2 other
    $net node 3 other
    $net node 4 box

    mklink $net 0 2  10Mb  2ms right-down
    mklink $net 1 2  10Mb  2ms right-up

    mklink $net 2 3 1.5Mb 10ms right-up
    mklink $net 3 4 1.5Mb 10ms right-down
    mklink $net 2 4 1.5Mb 10ms right

    $net color 0 blue
    $net color 1 red
    $net color 2 white

    $net queue 2 3 0
    $net queue 2 4 0

    # chart utilization from 1 to 2 width 5sec
    # chart avgutilization from 1 to 2 width 5sec
}

proc link-up {src dst} {
    ecolor $src $dst green
}

proc link-down {src dst} {
    ecolor $src $dst red
}

proc node-up src {
    ncolor $src green
}

proc node-down src {
    ncolor $src red
}
