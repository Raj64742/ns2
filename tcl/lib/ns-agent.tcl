Class Agent/Null -superclass Agent

Agent/Null instproc init args {
    eval $self next $args
}