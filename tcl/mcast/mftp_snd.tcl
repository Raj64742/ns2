#
# (c) 1997-98 StarBurst Communications Inc.
#
# THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# Author: Christoph Haenle, chris@cs.vu.nl
# File: mftp_snd.tcl
# Last change: Nov. 17, 1998
#
# This software may freely be used only for non-commercial purposes
#

Agent/MFTP/Snd set dtuSize_ 1424            ;# default size of DTUs (in bytes)
Agent/MFTP/Snd set dtusPerBlock_ 1472       ;# default number of DTUs per block
Agent/MFTP/Snd set dtusPerGroup_ 8          ;# default group size
Agent/MFTP/Snd set fileSize_ 1000000        ;# default file size in bytes
Agent/MFTP/Snd set readAheadBufsize_ 2097152;# default size of read-ahead buffer in bytes
Agent/MFTP/Snd set interval_ 512000         ;# default transmission rate is 512kbps
Agent/MFTP/Snd set txStatusLimit_ 100       ;# default max. number of consecutive status requests without NAK
Agent/MFTP/Snd set txStatusDelay_ 2         ;# default time to wait for status responses after a request before polling again
Agent/MFTP/Snd set rspBackoffWindow_ 1      ;# default max. time for receivers to wait before replying with nak(s) after a request
Agent/MFTP/Snd set reply_addr_ undefined    ; # application _must_ specify the sender address (i.e. the one
                                            ;# to which NAKs are unicast to). Default is "undefined"
Agent/MFTP/Snd set reply_port_ undefined

Agent/MFTP/Snd set nakCount_ 0
Agent/MFTP/Snd set seekCount_ 0             ;# number of disk seeks performed

Agent/MFTP/Snd instproc init {} {
    $self next
    $self instvar ns_ dtuSize_ dtusPerBlock_ dtusPerGroup_ fileSize_ 
    $self instvar reply_addr_ reply_port_ readAheadBufsize_ interval_ 
    $self instvar txStatusLimit_ txStatusDelay_ rspBackoffWindow_ nakCount_ 
    $self instvar seekCount_

    set ns_ [Simulator instance]
    foreach var { dtuSize_ dtusPerBlock_ dtusPerGroup_ fileSize_ \
                      readAheadBufsize_ interval_ txStatusLimit_ \
                      txStatusDelay_ rspBackoffWindow_ nakCount_ seekCount_ } {
        $self init-instvar $var
    }
}

# send-data does not get logged here. Rather, we get a callback-call "send-notify" from
# the C++ function se_data
Agent/MFTP/Snd instproc send-data { } {
    $self instvar ns_ interval_
    if { [$self cmd send data] != -1 } {
        # if not end-of-pass, re-schedule new "send-data" event
        $ns_ at [expr [$ns_ now] + $interval_] "$self send-data"
    }
}

Agent/MFTP/Snd instproc start {} {
    $self instvar node_ dst_addr_

    set dst_addr_ [expr $dst_addr_]           ;# get rid of possibly leading 0x etc.

    $self cmd start
    $node_ join-group $self $dst_addr_
    $self send-data
}

Agent/MFTP/Snd instproc pass-finished { CurrentPass NbBlocks } {
    $self instvar ns_ dtusPerGroup_ interval_ tx_status_requests_ rspBackoffWindow_

    set tx_status_requests_ 0       ;# number of consecutively sent status requests
    if { $CurrentPass >= $dtusPerGroup_ - 1 } {
        $self send status-req $CurrentPass 0 [expr $NbBlocks-1] $rspBackoffWindow_
    } else {
        # the first dtusPerGroup_ passes are sent without waiting for NAKs:
        # re-schedule new "send-data" event
        $ns_ at [expr [$ns_ now] + $interval_] "$self send-data"
    }
}


# send-status-req is called at end-of-pass, but only after the dtusPerGroup-th pass.
Agent/MFTP/Snd instproc send-status-req { CurrentPass blockLo blockHi rspBackoffWindow } {
    $self instvar ns_ tx_status_requests_ txStatusDelay_

    $self cmd send statreq $CurrentPass $blockLo $blockHi $rspBackoffWindow
    incr tx_status_requests_
    $ns_ at [expr [$ns_ now] + $txStatusDelay_] \
        "$self status-rsp-pending $CurrentPass $blockLo $blockHi"
}


# status-rsp-pending is called after the status-request packet is sent and
# if the status-rsp-pending-timer expires.
Agent/MFTP/Snd instproc status-rsp-pending { CurrentPass blockLo blockHi } {
    $self instvar nakCount_ tx_status_requests_ txStatusLimit_ rspBackoffWindow_

    # see if we have at least received 1 NAK
    # if yes, then start over with new pass, i.e. $self send-data
    # if no, send another status request, i.e. $self send status-req
    if { $nakCount_ > 0 } {
        # start over with next pass:
        set nakCount_ 0
        $self send-data
    } elseif { $tx_status_requests_ < $txStatusLimit_ } {
        # if we have not reached the maximum number of status-requests:
        $self send status-req $CurrentPass $blockLo $blockHi $rspBackoffWindow_
    } else {
        # Reached the maximum number of status requests
        $self done
    }
}


Agent/MFTP/Snd instproc recv { type args } {
    eval $self evTrace $proc $type $args
    eval $self $proc-$type $args
}

Agent/MFTP/Snd instproc send { type args } {
    eval $self evTrace $proc $type $args
    eval $self $proc-$type $args
}

# we get here whenever the sender sends a new data packet (callback from send_data)
Agent/MFTP/Snd instproc send-notify { args } {
}

Agent/MFTP/Snd instproc recv-nak { passNb block_nb nak_count} {
}


Agent/MFTP/Snd instproc done {} {
    # no further registering of events => end of transmission
}


Agent/MFTP/Snd instproc trace fd {
    $self instvar trace_
    set trace_ $fd
}


Agent/MFTP/Snd instproc delete {} {
}

Agent/MFTP/Snd instproc evTrace { op type args } {
    $self instvar trace_ ns_
    if [info exists trace_] {
	puts $trace_ [format "%7.4f [[$self set node_] id] $op-$type $args" [$ns_ now]]
    }
}

