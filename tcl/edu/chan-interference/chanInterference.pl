sub usage {
	print STDERR "usage: $0 <Base Trace file> <Base nam trace file> <output nam trace file>n";
	exit;
}

#@ARGV[0] - source trace file
#@ARGV[1] - source nam trace file
#@ARGV[2] - output nam trace file

open (Source, $ARGV[0]) or die "Cannot open $ARGV[0] : $!\n";
open (NamSource, $ARGV[1]) or die "Cannot open $ARGV[1] : $!\n";
open (Destination, ">$ARGV[2]") or die "Cannot open $ARGV[2]: $!\n";

$namline = <NamSource>;
while ($namline) { 
	if ($namline =~ /-i 3 -n green/) {
		print Destination $namline;
		print Destination 'v -t 0.000 -e sim_annotation 0.0 1 COLOR LEGEND : ', "\n";
		print Destination 'v -t 0.001 -e sim_annotation 0.001 2 Nodes turn green when they are sensing carrier ', "\n";
		print Destination 'v -t 0.002 -e sim_annotation 0.002 3 Nodes turn purple when they backoff ', "\n";
		print Destination 'v -t 0.003 -e sim_annotation 0.003 4 Nodes turn red when there is a collision ', "\n";
		print Destination 'v -t 0.10000000 -e set_rate_ext 0.200ms 1', "\n";
		last;
	}
	else
	{
		print Destination $namline;
		$namline = <NamSource>;
	}
}

$num_mov = 0;

$i =5;

$last_time = 0.1000;
$line = <Source>;
while ($line) {
	if ($line =~ /^M/) {
		@fields = split ' ', $line;
		@fields[3] =~ m/(\d+\.\d+)/;
		@fields[3] = $1;
		@fields[4] =~ m/(\d+\.\d+)/;
		@fields[4] = $1;
		@fields[6] =~ m/(\d+\.\d+)/;
		@fields[6] = $1;
		$t = (@fields[6] - @fields[3])/@fields[8];
		print Destination 'n -t ', @fields[1], ' -s ', @fields[2], ' -x ', @fields[3], ' -y ', @fields[4], ' -U ',@fields[8], ' -V 0.00 -T ', $t,"\n";

		print Destination 'v -t ', @fields[1], ' -e sim_annotation ', @fields[1],' ', $i,' Node 2 moves ',"\n";
		$last_time = @fields[1]+0.00005;
		$i++;
		print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' and hence is slightly out of range of ',"\n";
		$last_time = $last_time+0.00005;
		$i++;
		print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ', $i,' node 0   ',"\n";
		$i++;

		$num_mov ++;
		$last_time = $last_time + 0.08;
		$line = <Source>;
	}
	elsif ($num_mov < 1) {
		if ($line =~ /SENSING_CARRIER/) {
			@fields = split ' ', $line;
			$next_line = <Source>;
			if ($next_line =~ /BACKING_OFF/) {
				print "Does it enter here?\n";
				$other_node = 2;
				if (@fields[4] == 2)
				{
					$other_node = 0;
				}
				$t = @fields[2] +0.005;
				print Destination 'n -t ', @fields[2], ' -s ', @fields[4], ' -S COLOR -c green -o black -i green -I black ', "\n";
				print Destination 'n -t ', @fields[2], ' -s ', @fields[4], ' -S DLABEL -l "Carrier sense" -L ""', "\n";
				print Destination 'v -t ', @fields[2], ' -e sim_annotation ', @fields[2],' ', $i,' Since Nodes 0 and 2 are in ',"\n";
				$last_time = @fields[2]+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' range of each other, a node can ',"\n";
				$last_time = $last_time+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ', $i,' detect carrier if the other node ',"\n";
				$last_time = $last_time+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ', $i,' is sending data and hence it backs off.',"\n";
				$i++;
				$next_duration = @fields[2] + 0.01;

				print Destination 'n -t ', $next_duration, ' -s ', $other_node, ' -S COLOR -c green -o black -i green -I black ', "\n";
				print Destination 'n -t ', $next_duration, ' -s ', $other_node, ' -S DLABEL -l "Carrier Sense" -L ""', "\n";

				print Destination 'n -t ', $next_duration,' -s ', @fields[4], ' -S COLOR -c black -o green -i black -I green ', "\n";
				print Destination 'n -t ', $next_duration, ' -s ', @fields[4], ' -S DLABEL -l "" -L ""', "\n";
				print Destination '+ -t ', $next_duration, ' -s ', @fields[4], ' -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $next_duration, ' -s ', @fields[4], ' -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $next_duration, ' -s ', @fields[4], ' -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'r -t ', $next_duration, ' -s ', @fields[4], ' -d 1 -p message -e 2500 -a 1 ', "\n";

				$next_duration = $next_duration + 0.005;
				print Destination 'n -t ', $next_duration, ' -s ', $other_node, ' -S COLOR -c purple -o green -i purple -I green ', "\n";
				print Destination 'n -t ', $next_duration, ' -s ', $other_node, ' -S DLABEL -l "Backing off" -L ""', "\n";

				$next_duration = $next_duration + 0.005;
				print Destination 'n -t ',$next_duration,' -s ', $other_node,' -S COLOR -c green -o purple -i green -I purple ', "\n";
				print Destination 'n -t ',$next_duration,' -s ', $other_node,' -S DLABEL -l "Carrier sense" -L ""', "\n";
				$next_duration = $next_duration + 0.005;
				print Destination 'n -t ',$next_duration,' -s ', $other_node,' -S COLOR -c black -o green -i black -I green ', "\n";
				print Destination 'n -t ',$next_duration,' -s ', $other_node,' -S DLABEL -l "" -L ""', "\n";
				print Destination '+ -t ', $next_duration, ' -s ', $other_node, ' -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $next_duration, ' -s ', $other_node, ' -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $next_duration, ' -s ', $other_node, ' -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'r -t ', $next_duration, ' -s ', $other_node, ' -d 1 -p message -e 2500 -a 1 ', "\n";

				$last_time = $next_duration;
				$line = <Source>;
			}
			elsif ($next_line =~ /SENSING_CARRIER/) {
				@new_fields = split ' ', $next_line;
				print Destination 'n -t ', @new_fields[2] , ' -s 0 -S COLOR -c green -o black -i green -I black ',"\n";
				print Destination 'n -t ', @new_fields[2], ' -s 0 -S DLABEL -l "Carrier sense" -L ""', "\n";
				print Destination 'n -t ', @new_fields[2] , ' -s 2 -S COLOR -c green -o black -i green -I black ', "\n";
				print Destination 'n -t ', @new_fields[2], ' -s 2 -S DLABEL -l "Carrier Sense" -L ""' , "\n";

				print Destination 'v -t ', @new_fields[2], ' -e sim_annotation ', @new_fields[2],' ', $i,' Even though nodes 0 and 2 ',"\n";
				$last_time = @new_fields[2]+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' are in range of each other since they ',"\n";
				$last_time = $last_time+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ', $i,' do carrier sense at the same time they ',"\n";
				$last_time = $last_time+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ', $i,' are not able to detect carrier and hence',"\n";
				$last_time = $last_time+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ', $i,' their data packets collide at the destination',"\n";
				$i++;


				$duration = @new_fields[2] + 0.01;
				print Destination 'n -t ', $duration, ' -s 0 -S COLOR -c black -o green -i black -I green ', "\n";
				print Destination 'n -t ', $duration, ' -s 0 -S DLABEL -l "" -L ""', "\n";
				print Destination 'n -t ', $duration, ' -s 2 -S COLOR -c black -o green -i black -I green ', "\n";
				print Destination 'n -t ', $duration, ' -s 2 -S DLABEL -l "" -L ""', "\n";
				print Destination '+ -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '+ -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				$red_duration = $duration + 0.01;
				$red_end_duration = $red_duration + 0.01;
				print Destination 'n -t ', $red_duration, ' -s 1 -S COLOR -c red -o black -i red -I black ', "\n";
				print Destination 'n -t ', $red_duration, ' -s 1 -S DLABEL -l "Collision " -L ""', "\n";
				print Destination 'd -t ', $red_duration, ' -s 1 -d 2 -p message -e 5000 -a 8 ', "\n";
				print Destination 'n -t ', $red_end_duration, ' -s 1 -S COLOR -c black -o red -i black -I red ', "\n";
				print Destination 'n -t ', $red_end_duration, ' -s 1 -S DLABEL -l "" -L ""', "\n";
				$last_time = $red_end_duration;
				$line = <Source>;
			}
			else {
				$line = <Source>;
			}

			}
			else {
			$line = <Source>;
			}
		}
		elsif ($num_mov == 1) {
				$duration = $last_time + 0.01;
				print Destination 'n -t ', $duration, ' -s 0 -S COLOR -c green -o black -i green -I black ', "\n";
				print Destination 'n -t ', $duration, ' -s 0 -S DLABEL -l "Carrier Sense" -L ""', "\n";
				print Destination 'n -t ', $duration, ' -s 2 -S COLOR -c green -o black -i green -I black ', "\n";
				print Destination 'n -t ', $duration, ' -s 2 -S DLABEL -l "Carrier Sense" -L ""', "\n";
				print Destination '+ -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				$duration = $duration + 0.015;
				print Destination '+ -t ', $duration, ' -s 2 -d 1 -p message -e 1500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 2 -d 1 -p message -e 1500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 2 -d 1 -p message -e 1500 -a 1 ', "\n";
				print Destination '+ -t ', $duration,' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";

				print Destination '- -t ', $duration,' -s 0 -d 1 -p message -e 2500 -a 1', "\n";
				print Destination 'h -t ', $duration,' -s 0 -d 1 -p message -e 2500 -a 1', "\n";
				$duration = $duration + 0.01;
				print Destination 'n -t ', $duration, ' -s 0 -S COLOR -c black -o green -i black -I green ', "\n";
				print Destination 'n -t ', $duration, ' -s 0 -S DLABEL -l "" -L ""', "\n";
				print Destination 'n -t ', $duration, ' -s 2 -S COLOR -c black -o green -i black -I green ', "\n";
				print Destination 'n -t ', $duration, ' -s 2 -S DLABEL -l "" -L ""', "\n";

				$red_end_duration = $duration + 0.01;
				print Destination 'n -t ', $duration, ' -s 1 -S COLOR -c red -o black -i red -I black ', "\n";
				print Destination 'n -t ', $duration, ' -s 1 -S DLABEL -l "Collision " -L ""', "\n";
				print Destination 'd -t ', $duration, ' -s 1 -d 2 -p cbr -e 30000 -a 8 ', "\n";
				
				print Destination 'n -t ', $red_end_duration, ' -s 1 -S COLOR -c black -o red -i black -I red ', "\n";
				print Destination 'n -t ', $red_end_duration, ' -s 1 -S DLABEL -l "" -L ""', "\n";
				$last_time = $red_end_duration;

				$duration = $last_time + 0.01;
				print Destination 'n -t ', $duration, ' -s 0 -S COLOR -c green -o black -i green -I black ', "\n";
				print Destination 'n -t ', $duration, ' -s 0 -S DLABEL -l "Carrier Sense" -L ""', "\n";
				print Destination 'n -t ', $duration, ' -s 2 -S COLOR -c green -o black -i green -I black ', "\n";
				print Destination 'n -t ', $duration, ' -s 2 -S DLABEL -l "Carrier Sense" -L ""', "\n";
				print Destination '+ -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				$duration = $duration + 0.015;
				print Destination '+ -t ', $duration, ' -s 2 -d 1 -p message -e 1500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 2 -d 1 -p message -e 1500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 2 -d 1 -p message -e 1500 -a 1 ', "\n";
				print Destination '+ -t ', $duration,' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";

				print Destination '- -t ', $duration,' -s 0 -d 1 -p message -e 2500 -a 1', "\n";
				print Destination 'h -t ', $duration,' -s 0 -d 1 -p message -e 2500 -a 1', "\n";
				$duration = $duration + 0.01;
				print Destination 'n -t ', $duration, ' -s 0 -S COLOR -c black -o green -i black -I green ', "\n";
				print Destination 'n -t ', $duration, ' -s 0 -S DLABEL -l "" -L ""', "\n";
				print Destination 'n -t ', $duration, ' -s 2 -S COLOR -c black -o green -i black -I green ', "\n";
				print Destination 'n -t ', $duration, ' -s 2 -S DLABEL -l "" -L ""', "\n";

				$red_end_duration = $duration + 0.01;
				print Destination 'n -t ', $duration, ' -s 1 -S COLOR -c red -o black -i red -I black ', "\n";
				print Destination 'n -t ', $duration, ' -s 1 -S DLABEL -l "Collision " -L ""', "\n";
				print Destination 'd -t ', $duration, ' -s 1 -d 2 -p cbr -e 30000 -a 8 ', "\n";
				
				print Destination 'n -t ', $red_end_duration, ' -s 1 -S COLOR -c black -o red -i black -I red ', "\n";
				print Destination 'n -t ', $red_end_duration, ' -s 1 -S DLABEL -l "" -L ""', "\n";
				$last_time = $red_end_duration;

				$line = <Source>;
				$num_mov ++;


		}
		else {
			$line = <Source>;
		}
}