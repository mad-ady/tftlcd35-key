#!/usr/bin/perl
use strict;
use warnings;
use Log::Log4perl qw(:easy);
use Time::HiRes qw(usleep);
use Data::Dumper;

# Dependencies
# sudo apt-get install liblog-log4perl-perl

# Keyboard code for Odroid 3.5" TFT buttons with multiclick support
# The code generates a virtual keyboard and outputs some keycodes 
# which can be further processed with something like triggerhappy


# logging goes to stderr
Log::Log4perl->easy_init($DEBUG);
my $logger = Log::Log4perl->get_logger();

#configuration
my $updatePeriod = 200_000; #check keys every $updatePeriod microseconds
#define the thresholds for each button for each platform
my %adc = (
    'C1' => {
        'KEY1' => 5,
        'KEY2' => 515,
        'KEY3' => 680,
        'KEY4' => 770,
        'tollerance' => 20,
        'node' => '/sys/class/saradc/saradc_ch0',
        },
    'C2' => {
        'KEY1' => 5,
        'KEY2' => 515,
        'KEY3' => 680,
        'KEY4' => 770,
        'tollerance' => 20,
        'node' => '/sys/class/saradc/ch0',
        },
    'XU' => {
        'KEY1' => 0,
        'KEY2' => 2030,
        'KEY3' => 2695,
        'KEY4' => 3014,
        'tollerance' => 100,
        'node' => '/sys/devices/12d10000.adc/iio:device0/in_voltage3_raw',
        }
);
my $platform = 'none'; #updated in main() at runtime

sub detect_platform {
    open FILE, "/proc/cpuinfo" or die "Unable to read /proc/cpuinfo";
    my $platform = "none";
    while(<FILE>){
        my $line = $_;
        if($line=~/ODROID-C[0-1]/){
            $platform = "C1";
            last;
        }
        if($line=~/ODROID-C2/){
            $platform = "C2";
            last;
        }
        if($line=~/ODROID-XU[3-4]/ || $line=~/EXYNOS/){
            $platform = "XU";
            last;
        }
    }
    close FILE;
    if($platform eq 'none'){
        $logger->fatal("Unsupported platform detected");
        die "Unsupported platform detected";
    }
    return $platform
}

sub compareKEY{
    my $key = shift;
    my $status = shift;
    
    my $compare = 0;
    if ($key - $status < $adc{$platform}{'tollerance'} && $key - $status > - $adc{$platform}{'tollerance'}) {
        $compare = 1;
    }

    return $compare;
}

#read an instant value from the ADC node
sub analogRead{
    open ADC, "$adc{$platform}{'node'}" or die "Unable to open $adc{$platform}{'node'}";
    my $value = <ADC>;
    close ADC;
    return $value;
}

sub key_update {
    my $raw_value = analogRead();
    $logger->debug("Read $raw_value");
    foreach my $key (keys %{$adc{$platform}}){
        next if ($key!~/KEY/);
        my $keyIsPressed = compareKEY($raw_value, $adc{$platform}{$key});
        if($keyIsPressed){
            $logger->info("$key is pressed");
        }
    }
}

sub main {
    $platform = detect_platform();
    $logger->debug("Running on platform $platform");
    print Dumper(\%adc);
    #read keys from the keypad
    while(1){
        key_update();
        usleep($updatePeriod);
    }
}


#actual execution
main();
