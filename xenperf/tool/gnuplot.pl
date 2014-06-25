#!/usr/bin/perl -wl
use strict;
use Digest::SHA qw(sha1_hex);
use File::Temp qw(tempfile);
use Getopt::Long;


my $program = $0;
$program =~ s|^(.*/)*(.*?)(\.pl)?$|$2|;


sub _die
{
    my $msg = shift || "unexpected error";
    my $ret = shift || 1;

    print STDERR "$program: $msg";
    exit $ret;
}


my $line;
my $base;
my $delta;
my $style;
my $index;
my @fields;

my $fd;
my ($datafd, $data);
my ($gnuplotfd, $gnuplot);

my $x11;
my $verbose;
my $title = '';
my $logscale;
my $filledcurve;
my %bases;
my %events;


Getopt::Long::Configure("bundling");
GetOptions (
    'verbose|v' => \$verbose,
    'logscale|l' => \$logscale,
    'filledcurve|f' => \$filledcurve,
    'x11|x' => \$x11,
    'color=s' => \%bases,
    'event=s' => \%events,
    'title|t=s' => \$title
    );


$line = <>;
$line =~ s/^#\s*time\s+// || _die "invalid data header: time field";
@fields = split /\s+/, $line;


($datafd, $data) = tempfile;
while (<>) {
    printf $datafd $_;
}
close $datafd;


($gnuplotfd, $gnuplot) = tempfile;
printf $gnuplotfd <<"EOF";
set title '$title'
set y2tics mirror
set grid ytics
set key spacing .5 font ',8'
set key center outside bottom
set key maxrows 4
set xlabel 'time (s)'
EOF

if ($logscale) {
    print $gnuplotfd "set logscale y";
    print $gnuplotfd "set logscale y2";
}

if ($x11) {
    print $gnuplotfd "set terminal X11";
} else {
    print $gnuplotfd "set terminal pdf";
}

printf $gnuplotfd 'plot ';
$index = 2;
foreach (@fields) {
    $_ =~ /^\s*0x([0-9a-fA-F]+):([0-9a-fA-F]+)\((.*)\)\s*$/;
    $base = hex($bases{"$1:$2"} || substr(sha1_hex($1 . ':' . $2), 0, 6));
    $delta = hex(substr(sha1_hex($3), 0, 6));
    $delta = (($delta | ($delta << 4)) & 0x3f3f3f0) >> 4;
    $base += $delta;
    $base = sprintf '%x', $base;

    if ($filledcurve) {
	$style = 'filledcurve x1';
    } else {
	$style = 'lines';
    }

    print $gnuplotfd "'$data' using $index with $style title '"
	. ($events{"$1:$2"} || "0x$1:$2") . " (core $3)' "
	. "lc rgb '#$base', \\";
    $index++;
}

if ($x11) {
    printf $gnuplotfd "\npause 5\n";
}

close $gnuplotfd;


system 'cat', $gnuplot if ($verbose);
system "gnuplot < $gnuplot";

unlink $data || die "$!";
unlink $gnuplot;
