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
# File: mftp_rcv.tcl
# Last change: Dec. 14, 1998
# This software may freely be used only for non-commercial purposes
#

# standard settings:
Agent/MFTP/Rcv set dtuSize_ 1424
Agent/MFTP/Rcv set dtusPerBlock_ 1472
Agent/MFTP/Rcv set dtusPerGroup_ 8
Agent/MFTP/Rcv set fileSize_ 1000000
Agent/MFTP/Rcv set nakCount_ 0
Agent/MFTP/Rcv set seekCount_ 0
Agent/MFTP/Rcv set reply_addr_ 0           ; # unicast reply addr (=address of server)
Agent/MFTP/Rcv set reply_port_ 0           ; # unicast reply addr (=address of server)

Agent/MFTP/Rcv instproc init {} {
    $self next
    $self instvar ns_ dtuSize_ dtusPerBlock_ dtusPerGroup_ fileSize_ 
    $self instvar reply_addr_ reply_port_ nakCount_ seekCount_

    set ns_ [Simulator instance]
    foreach var { dtuSize_ dtusPerBlock_ dtusPerGroup_ fileSize_ reply_addr_ reply_port_ nakCount_ seekCount_} {
        $self init-instvar $var
    }
}

Agent/MFTP/Rcv instproc start {} {
    $self instvar node_ dst_addr_

    set dst_addr_ [expr $dst_addr_]           ;# get rid of possibly leading 0x etc.
    $self cmd start
    $node_ join-group $self $dst_addr_
}

Agent/MFTP/Rcv instproc delete {} {
}

Agent/MFTP/Rcv instproc done-notify { args } {
    # File reception completed

    $self instvar node_ dst_addr_
    eval $self evTrace done notify $args
    $node_ leave-group $self $dst_addr_
}

Agent/MFTP/Rcv instproc recv { type args } {
    eval $self evTrace $proc $type $args
    eval $self recv-$type $args
}

#Agent/MFTP/Rcv instproc send { type args } {
#    eval $self evTrace $proc $type $args
#    eval $self send-$type $args
#}

Agent/MFTP/Rcv instproc recv-dependent { CurrentPass CurrentGroup CwPat } {
    # discard packet due to a linear dependency (no action)
}

Agent/MFTP/Rcv instproc recv-group-full { CurrentPass CurrentGroup CwPat } {
    # discard packet due to a full group (no action)
}

Agent/MFTP/Rcv instproc recv-useful { CurrentPass CurrentGroup CwPat } {
    # receive useful packet
}

Agent/MFTP/Rcv instproc recv-status-req { passNb blockLo blockHi txStatusDelay } {
    $self instvar ns_
    set backoff [uniform 0 $txStatusDelay]
    $ns_ at [expr [$ns_ now] + $backoff] "$self send-nak [list $passNb $blockLo $blockHi]"
}

Agent/MFTP/Rcv instproc send-nak { passNb blockLo blockHi } {
    while { $blockLo <= $blockHi } {
        set bit_count [$self cmd send nak $passNb $blockLo]
        $self evTrace send nak $passNb $blockLo $bit_count
        incr blockLo
    }
}

Agent/MFTP/Rcv instproc trace fd {
    $self instvar trace_
    set trace_ $fd
}

Agent/MFTP/Rcv instproc evTrace { op type args } {
    $self instvar trace_ ns_
    if [info exists trace_] {
	puts $trace_ [format "%7.4f [[$self set node_] id] $op-$type $args" [$ns_ now]]
    }
}

