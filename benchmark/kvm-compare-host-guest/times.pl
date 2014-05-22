#!/usr/bin/perl -wl
use strict;
use File::Temp qw(tempfile);


my $fd;
my $path;
my $command;

unless (@ARGV) {
    print STDERR "syntax: $0 directories...";
    print STDERR "format logs in given directories and produce a graph";
    print STDERR "the result is a pdf written in the stdout";
    exit 1;
}


# Data formatting =============================================================

($fd, $path) = tempfile;
print STDERR "formatting data in '$path'...";

print $fd "name\tfirst-touch\tinterleaved";

for (@ARGV) {
    next unless (-d "$_-I" && -d "$_-NI");
    print STDERR "processing $_";

    my $real = 0;
    my $user = 0;
    my $sys  = 0;
    my $tot  = 0;
    
    foreach (split '\n', `ls $_-NI/*.log`) {
	open my $fd, '<', $_ || die "cannot open '$_': $!";

	while (<$fd>) {
	    /^\s*(\w+)\s*(\d+)m(\d+)\.(\d+)s\s*$/ || next;
	    eval "\$$1 += $2 * 60 + $3.$4;";
	}

	close $fd;
	$tot += 1;
    }

    $real /= $tot;
    $user /= $tot;
    $sys /= $tot;

    printf $fd "$_\t$real\t";

    $real = 0;
    $user = 0;
    $sys  = 0;
    $tot  = 0;
    
    foreach (split '\n', `ls $_-I/*.log`) {
	open my $fd, '<', $_ || die "cannot open '$_': $!";

	while (<$fd>) {
	    /^\s*(\w+)\s*(\d+)m(\d+)\.(\d+)s\s*$/ || next;
	    eval "\$$1 += $2 * 60 + $3.$4;";
	}

	close $fd;
	$tot += 1;
    }

    $real /= $tot;
    $user /= $tot;
    $sys /= $tot;

    print $fd "$real";
}

close $fd;

print STDERR "cat $path";
system "cat $path | column -t >&2";

# Data plotting ===============================================================

print STDERR "plotting data to stdout...";

$command = <<"GNUPLOT";
set title 'streamcluster 10 20 128 1000000 200000 5000 - - 48'
set ylabel 'time (s)'
set xlabel 'virtualization level & numa policy'
set key left
set grid ytics
set yrange [0:800]
set xtics rotate by -25
set style data histogram
set style fill solid border
set style histogram clustered
set terminal pdf
plot for [COL=2:3] '$path' using COL:xticlabel(1) title columnheader
GNUPLOT

open $fd, '|-', 'gnuplot' || (unlink $path && die "$!");
print $fd $command;
close $fd;

unlink $path;
