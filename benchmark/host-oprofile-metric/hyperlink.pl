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

print $fd "name\thyper0NI\thyper3NI\thyper0I\thyper3I";

for (@ARGV) {
    next unless (-d "$_-I" && -d "$_-NI");
    print STDERR "processing $_";

    my $hyp0 = 0;
    my $hyp3 = 0;
    my $tot  = 0;
    
    foreach (split '\n', `ls $_-NI/opreport*.log`) {
	open my $fd, '<', $_ || die "cannot open '$_': $!";

	while (<$fd>) {
	    /^\s*(\d+)\s*([\d.]+)\s*(\d+)\s*([\d.]+)\s*streamcluster\s*$/ || next;
	    $hyp0 += $1;
	    $hyp3 += $3;
	}

	close $fd;
	$tot += 1;
    }

    $hyp0 /= $tot;
    $hyp3 /= $tot;

    printf $fd "$_\t$hyp0\t$hyp3\t";

    $hyp0 = 0;
    $hyp3 = 0;
    $tot  = 0;
    
    foreach (split '\n', `ls $_-I/opreport*.log`) {
	open my $fd, '<', $_ || die "cannot open '$_': $!";

	while (<$fd>) {
	    /^\s*(\d+)\s*([\d.]+)\s*(\d+)\s*([\d.]+)\s*streamcluster\s*$/ || next;
	    $hyp0 += $1;
	    $hyp3 += $3;
	}

	close $fd;
	$tot += 1;
    }

    $hyp0 /= $tot;
    $hyp3 /= $tot;

    print $fd "$hyp0\t$hyp3";
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
plot for [COL=2:5] '$path' using COL:xticlabel(1) title columnheader
GNUPLOT

open $fd, '|-', 'gnuplot' || (unlink $path && die "$!");
print $fd $command;
close $fd;

unlink $path;
