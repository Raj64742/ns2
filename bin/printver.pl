eval 'exec perl -S $0 ${1+"$@"}'
    if 0;

# if given at least 1 argument, the first one is the string to be printed

if ($#ARGV < 0) {
    print 'char version_string = ';
} else {
    print $ARGV[0];
    shift;
}

print '"';
while (<>) {
    chop;
    print $_;
}
print "\";\n";
