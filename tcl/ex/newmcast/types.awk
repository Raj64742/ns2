BEGIN { }
$1 ~ /[-]/ {
  found = 0;
  for (j = 2; j < $NF; j++) {
	if ($j == "-s") {
		a = j + 1;
	}
	if ($j == "-d") {
		b = j + 1;
	}
	if ($j == "-c") {
	  c = j + 1;
	  if ($c != 30 && $c != 31 && $c != 32 && $c != 33 && $c != 34) {
		continue;
	  }
	  found = 1;
	}
  }
	if (found) {
		key = $a ":" $b ":" $c
		A[key] ++
	}
}
END {
	for (i in A) {
		split(i,x,":") 
		print x[1],x[2],x[3],A[i]
	}
}
