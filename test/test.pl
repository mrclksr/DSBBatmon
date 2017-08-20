#!/usr/local/bin/perl
#
# Generate randomly changing battery states (c = charge, d = discharge),
# simulate adding and removing battery to/from system, and send valid and
# invalid devd ACPI events. 
#
#
# qmake TEST=1 && make
# perl test.pl | ./dsbbatmon
#

use strict;
use IO::Handle;

my $spath = '/tmp/dsbbatmon-test.socket';
my $upath = '/tmp/dsbbatmon-test.presence';

my @ev = ('!system=ACPI subsystem=ACAD', '!system=ACPI subsystem=CMBAT',
	  '!system=FOO', 'system=ACPI', '!system=ACPI	subsystem=FOO');
my @st = ('c', 'd');
my $fh;

sub _rand {
	my $f;
	my $x;
	open($f, "<", "/dev/random") || die "$!\n";
	read($f, $x, 4);
	$x = hex(unpack("H*", $x)); 
	close($f);
	return $x;
}

sub set_presence {
	my $f;
	open($f, '+>', $upath) || die "$!\n";
	print $f $_[0];
	close($f);
}

set_presence(_rand() % 2);

system("rm -f $spath");
open($fh, "| nc -l -U $spath") || die "$!\n";
for (;;) {
	my $m = _rand() % 1000;
	my $n = _rand() % 100;
	my $k = _rand() % 2;
	my $j = _rand() % 10;
	my $i = _rand() % @ev;

	if ($m % 25 == 0) {
		set_presence(int(_rand() % 2));
	}
	if ($n % 7 == 0) {
		print "$st[$k]\n";
		STDOUT->flush();
		$fh->flush();
	} 
	sleep($j);
	print $fh "$ev[$i]\n";
	$fh->flush();
}

