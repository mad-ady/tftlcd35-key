#!/usr/bin/perl
use strict;
use warnings;
use Log::Log4perl qw(:easy);

# Dependencies
# sudo apt-get install liblog-log4perl-perl

# Keyboard code for Odroid 3.5" TFT buttons with multiclick support
# The code generates a virtual keyboard and outputs some keycodes 
# which can be further processed with something like triggerhappy


# logging goes to stderr
Log::Log4perl->easy_init($DEBUG);
my $logger = Log::Log4perl->get_logger();

#configuration

sub detect_platform {
    open FILE, "/proc/cpuinfo" or die "Unable to read /proc/cpuinfo";
    my $platform = "none";
    while(<FILE>){
        my $line = $_;
        if($line=~/ODROID-C[0-2]/){
            $platform = "C";
            last;
        }
        if($line=~/ODROID-XU[3-4]/ || $line=~/EXYNOS/){
            $platform = "XU";
            last;
        }
    }
    if($platform eq 'none'){
        $logger->fatal("Unsupported platform detected");
        die "Unsupported platform detected";
    }
    return $platform
}


sub main {
    my $platform = detect_platform();
    $logger->debug("Running on platform $platform");
}


#actual execution
main();
