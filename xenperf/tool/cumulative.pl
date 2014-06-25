#!/bin/perl -wl
use strict;


my $program = $0;
$program =~ s|^(.*/)*(.*?)(\.pl)?$|$2|;


sub _die
{
    my $msg = shift || "unexpected error";
    my $ret = shift || 1;

    print STDERR "$program: $msg";
    exit $ret;
}


my $line = <>;         # current line
my $index;             # generic index
my @fields;            # fields of a line

my @acc;
my $sum;

printf $line;

while (<>) {
    @fields = split /\t/, $_;

    $line = $fields[0];

    $sum = 0;
    @acc = ();
    foreach (reverse @fields[1 .. $#fields]) {
	if (/^\s*$/) {
	    push @acc, '';
	} else {
	    $sum += $_;
	    push @acc, $sum;
	}
    }

    foreach (reverse @acc) {
	$line .= "\t";
	/^\s*$/ and next;
	$line .= sprintf '%.1f', $_;
    }

    print $line;
}
