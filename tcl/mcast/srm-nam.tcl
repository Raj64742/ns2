# 
#  Copyright (c) 1997 by the University of Southern California
#  All rights reserved.
# 
#  Permission to use, copy, modify, and distribute this software and its
#  documentation in source and binary forms for non-commercial purposes
#  and without fee is hereby granted, provided that the above copyright
#  notice appear in all copies and that both the copyright notice and
#  this permission notice appear in supporting documentation. and that
#  any documentation, advertising materials, and other materials related
#  to such distribution and use acknowledge that the software was
#  developed by the University of Southern California, Information
#  Sciences Institute.  The name of the University may not be used to
#  endorse or promote products derived from this software without
#  specific prior written permission.
# 
#  THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
#  the suitability of this software for any purpose.  THIS SOFTWARE IS
#  PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
#  INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 
#  Other copyrights might apply to parts of this software and are so
#  noted when applicable.
# 
#	Author:		Kannan Varadhan	<kannan@isi.edu>
#	Version Date:	Mon Jun 30 15:51:33 PDT 1997
#

if ![info exists ctrlFid] {
    set ctrlFid 39
}

set sessFid [incr ctrlFid]
set rqstFid [incr ctrlFid]
set reprFid [incr ctrlFid]

Agent/SRM instproc send {type args} {
#    eval $self evTrace $proc $type $args
    global sessFid rqstFid reprFid

    set fid [$self set fid_]
    switch $type {
    session	{ $self set fid_ $sessFid }
    request	{ $self set fid_ $rqstFid }
    repair	{ $self set fid_ $reprFid }
    }
    eval $self cmd send $type $args
    $self set fid_ $fid
}
