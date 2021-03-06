#!/usr/bin/perl
use strict;
use warnings;
my @key=qw/KEY1 KEY2 KEY3 KEY4/;
print '
# Configuration sample for tftlcd35_key.pl

# Set logging level (logging goes to STDERR/journalctl). Allowed values are: DEBUG, INFO, WARN, ERROR, FATAL                                                    
                                                                                
logging: DEBUG

# Set updatePeriod to the number of microseconds between key presses checks
# Default is 200ms between key presses

updatePeriod: 200000

# Set bufferSize to how many keys to keep in memory when analizing a multiple key press.
# The larger the value, the more time you have to wait until any action executes

bufferSize: 10

# Set longPress to a fraction - how many times does a key have to appear in a sequence before considering it a long press.
# The default is 0.7 which means 70% of the keys in bufferSize have to be the same to register as a long press

longPress: 0.7

# The following section defines possible key sequences and an action that should be executed in the background
# Note that the command runs as the same user you run tftlcd35_key.pl by default

# Example commands (one line each):
#KEY1: logger "Key1 has been pressed"
#KEY2: logger "Key2 has been pressed"; logger "Two commands have been executed"
#KEY3: su -u odroid -c "logger \'This command is run as a different user (uid $EUID)\'"
#KEY4: DISPLAY=:0 xeyes

';
foreach my $level1 (@key){
	print "$level1: \n";
	foreach my $level2 (@key){
		last;
		print "$level1-$level2: \n";
		foreach my $level3 (@key){
			last;
			print "$level1-$level2-$level3: \n";
		}
	}
	print "LONG$level1: \n";
}
