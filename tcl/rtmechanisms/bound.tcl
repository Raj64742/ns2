# return number of drops necessary for probability prob given c and p
proc drop_bound {prob c p type} {
  # equation not defined in these cases
  if {$p <= 0 || $p >= 1} {
    return 0
  }
  if {$c < 1} {
    return 0
  }

  # Chernoff bound calcs
  set cp [expr $c * $p]
  set lnp [expr [expr log($prob)]]
  set 1mp [expr 1.0 - $p]
  set 1mcp [expr 1.0 - $cp]
  set pdc [expr $p / $c]
  set 1mpdc [expr 1.0 - $pdc]
  set 1dc [expr 1.0 / $c]
  set a [expr [expr pow($1dc,$cp)] * [expr pow([expr $1mp-$1mcp],$1mcp)]] 
  set b [expr [expr pow($c,$pdc)] * [expr pow([expr $1mp/$1mpdc],$1mpdc)]] 
  set n_upper [expr $lnp / [expr log($a)]] 
  set n_lower [expr $lnp / [expr log($b)]]

  switch -exact -- $type {
    upper {
      set bound $n_upper
    }
    lower {
      set bound $n_lower
    }
    both {
      if {$n_lower >= $n_upper} {
        set bound $n_lower
      } else {
        set bound $n_upper
      }
    } 
  }
  return $bound
}
