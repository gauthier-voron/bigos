#!/usr/bin/perl -wl
use strict;
use List::Util qw(first);

my $pid;
my $cpumask;
my $nextcpu;
my $cpucount = first { /^\d+$/ } map { /^CPU\(s\):\s+(\d+)\s*$/ && $1 } split '\n', `lscpu`;
my @process = map { /^\D*(\d+)/ && $1 } grep { /qemu/ } split '\n', `ps aux`;
my @threads = map { /^\D*(\d+)\D+(\d+)/ && "$1:$2" } grep { /qemu/ } split '\n', `ps aux -L`;
my %thread_map;

foreach (@threads) {
    /(\d+):(\d+)/;
    $thread_map{$2} = $1;
}

@process = grep { $_ != $$ } @process;

$nextcpu = 0;
foreach $pid (sort keys %thread_map) {
    if ($thread_map{$pid} != $$ && $thread_map{$pid} != $pid) {
	$cpumask = sprintf '0x%012x', 1 << $nextcpu;

	die "$!" if (system ('taskset', '-p', "$cpumask", "$pid") != 0);

	$nextcpu = ($nextcpu + 1) % $cpucount;
    }
}
