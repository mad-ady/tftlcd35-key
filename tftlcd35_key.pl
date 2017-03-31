#!/usr/bin/perl
use strict;
use warnings;
use Log::Log4perl qw(:easy);
use Log::Log4perl::Level;
use Time::HiRes qw(usleep);
use Data::Dumper;
use Proc::Background;
use Config::YAML;

# Dependencies
# sudo apt-get install liblog-log4perl-perl libproc-background-perl libconfig-yaml-perl

# Keyboard code for Odroid 3.5" TFT buttons with multiclick support
# The code executes external commands as a response to button presses

my $config;
#load configuration
if(defined $ARGV[0] && -f $ARGV[0]){
    $config = Config::YAML->new(config => $ARGV[0]);    
}
else{
    die "Usage: $0 tftlcd_config.yaml";
}

#configuration
my $updatePeriod = $config->{updatePeriod} || 200_000; #check keys every $updatePeriod microseconds
my $bufferSize = $config->{bufferSize} || 10; #how many keypresses to keep in a buffer before processing a composite event. A large value results in a delay of an action
my $longPress = $config->{longPress} || 0.7; #how much of the buffer must contain the same key before it is considered a long press event? Eg 70%
my $logLevel = $config->{logging} || "DEBUG"; #the logging level

# logging goes to stderr
my $level = Log::Log4perl::Level::to_priority($logLevel);
Log::Log4perl->easy_init($level);
my $logger = Log::Log4perl->get_logger(); 

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

my @buffer;

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
    return if($raw_value < 0);
    $logger->debug("Read $raw_value");
    my $oneKeyPressed = 0;
    
    foreach my $key (keys %{$adc{$platform}}){
        next if ($key!~/KEY/);
        my $keyIsPressed = compareKEY($raw_value, $adc{$platform}{$key});
        if($keyIsPressed){
            $logger->debug("$key is pressed");
            $oneKeyPressed = 1;
            #push the key to the buffer
            push @buffer, $key;
        }
    }
    if(scalar(@buffer)){
        $logger->debug("Buffer contains: ".join("|", @buffer));
        if(! $oneKeyPressed){
            push @buffer, " ";
        }
    }
    
    #if the buffer gets too big, interpret the result
    if(scalar(@buffer) > $bufferSize){
        
        #long presses take precedence - was there a long press detected?
        my $long = longPress(@buffer);
        my $sequence = undef;
        if($long ne "0"){
            $sequence = $long;
        }
        else{
            $sequence = keySequence(@buffer);
        }
        
        #do something based on the key pressed
        doSomething($sequence);
        #reset buffer when done
        @buffer = ();
        
        #maybe do a little sleep to debounce subsequent keys?
    }
}

sub keySequence {
    my @buffer = @_;
    $logger->info("Processing buffer: ".join("|", @buffer));
    
    my @sequence;
    my $current = undef;
    my $previous = undef;
    foreach my $item(@buffer){
        #shift things around one position
        $previous = $current;
        $current = $item;
        
        if(defined $previous){
            if($current ne $previous){
                if($current ne ' '){
                    #add it to the sequence list
                    push @sequence, $current;
                }
                else{
                    push @sequence, "-";
                }
            }
            else{
                #if the current entry is the same as previous, ignore it (duplicated key)
            }
        }
        else{
            #first reading - save the key if it's not space (it couldn't be space anyway)
            if($current ne ' '){
                push @sequence, $current;
            }
        }
    }
    #collapse the array into a string
    my $seq = join('', @sequence);
    #cut out beginning or ending '-'
    $seq=~s/^-|-$//g;
    $logger->info("Identified key sequence: $seq");
    return $seq;
}

sub longPress {
    my @buffer = @_;
    
    #see if any one key has been held down for at least $longPress
    my %keys;
    foreach my $item (@buffer){
        if($item=~/KEY/){
            $keys{$item}++;
        }
    }
    foreach my $key (sort {$keys{$a} <=> $keys{$b}} keys %keys ){
        if($keys{$key} >= $longPress * scalar(@buffer)){
            $logger->info("Identified long press for $key ($keys{$key})");
            return "LONG$key";
        }
    }
    return 0;
}

sub doSomething {
    my $sequence = shift;
    if(defined $config->{$sequence}){
        $logger->info("Doing something for $sequence");
        $logger->debug("Running ".$config->{$sequence});
        my $proc = Proc::Background->new($config->{$sequence});
        $logger->debug("Started process ".$proc->pid);
    }
    else{
        $logger->error("No sequence defined $sequence");
    }
    
}


sub main {
    $platform = detect_platform();
    $logger->info("Running on platform $platform");
    print Dumper(\%adc);
    #read keys from the keypad
    while(1){
        key_update();
        usleep($updatePeriod);
    }
}


#actual execution
main();
