Trace instproc init type {
    $self next $type
    $self instvar type_
    set type_ $type
}

Trace instproc format args {
    $self instvar type_ fp_ src_ dst_

    set ns [Simulator info instances]
    eval puts $fp_ "\"$type_ [$ns now] $args\""
}

Trace instproc attach fp {
    $self instvar fp_

    set fp_ $fp
    $self cmd attach $fp_
}

Class Trace/Hop -superclass Trace
Trace/Hop instproc init {} {
    $self next "h"
}

Class Trace/Enque -superclass Trace
Trace/Enque instproc init {} {
    $self next "+"
}

Class Trace/Deque -superclass Trace
Trace/Deque instproc init {} {
    $self next "-"
}

Class Trace/Drop -superclass Trace
Trace/Drop instproc init {} {
    $self next "d"
}

Class Trace/Generic -superclass Trace
Trace/Generic instproc init {} {
    $self next "v"
}
