#Code to generate random numbers here
proc exponential {} {
    expr - log ([uniform 0 1])
}

proc uniform {a b} {
    expr $a + (($b - $a) * ([ns-random] * 1.0 / 0x7fffffff))
}

proc integer k {
    expr [ns-random] % abk($k)
}

