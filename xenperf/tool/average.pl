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


my @field_to_event;
my %event_to_field;
my %event_to_core;

my $line = <>;         # current line
my $index;             # generic index
my @fields;            # fields of a line

my %event_sum;
my %event_count;


$line =~ s/^#\s*time\s+// || _die "invalid data header: time field";
@fields = split /\s+/, $line;

push @field_to_event, 'time';
$index = 1;

foreach (@fields) {
    /^0x([0-9a-fA-F]+):([0-9a-fA-F]+)\(([0-9]+)\)$/
	|| _die "invalid data header: field '$_'";
    $event_to_field{"$1:$2"} .= ":$index";
    $event_to_core{"$1:$2"} .= ":$3";
    push @field_to_event, "$1:$2";
    $index++;
}

foreach (keys %event_to_field) {
    $event_to_field{$_} =~ s/^.//;
    $event_to_core{$_} =~ s/^.//;
}


$line = "#time";
foreach (sort keys %event_to_core) {
    $line .= "\t0x$_(" . $event_to_core{$_} . ')';
}
print $line;


while (<>) {
    @fields = split /\t/, $_;
    $event_count{$_} = $event_sum{$_} = 0 foreach (keys %event_to_field);

    $line = $fields[0];

    $index = 0;
    foreach (@fields[1 .. $#fields]) {
	$index++;
	/^\s*$/ and next;
	
	$event_count{$field_to_event[$index]}++;
	$event_sum{$field_to_event[$index]} += $_;
    }

    foreach (sort keys %event_to_field) {
	$line .= "\t";
	$event_count{$_} == 0 and next;
	$line .= (sprintf '%.1f', ($event_sum{$_} / $event_count{$_}));
    }

    print $line;
}
