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

my $sum;
my $skip;

printf $line;

while (<>) {
    @fields = split /\t/, $_;

    $line = $fields[0];

    $skip = 0;
    $sum = 0;

    foreach (@fields[1 .. $#fields]) {
	/^\s*$/ and $skip=1 and last;
	$sum += $_;
    }
    $skip and next;
    ($sum == 0) and next;

    foreach (@fields[1 .. $#fields]) {
	$line .= (sprintf "\t%.1f", (($_ * 100) / $sum));
    }

    print $line;
}
