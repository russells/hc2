#!/usr/bin/perl

# Monitor the serial output from HC, and print the difference between the
# current time and HC's time.
#
# We don't care that much about the difference, but we want to see how the
# difference changes over a day or more.
#
# Beware of HC's adjustment algorithm.  It does not alter the clock rate.
# Instead it changes the length of some seconds by 1/8 second through the
# day.  So for the difference comparison to be meaningful, it should be
# compared at exactly the same time of day.
#
# This method of adjustment is not really suitable for end users.

use POSIX qw( mktime strftime );
use Time::HiRes qw( gettimeofday );

my $minute_counter = -1;

my $cal_minute;
my $first_date;
my $first_diff;


while (<>) {

	if (m/^TIME=([0-2][0-9])([0-5][0-9])$/) {

		$minute_counter += 1;

		($t, $us) = gettimeofday();
		print "t   =$t us=$us\n";
		($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
		    = localtime($t);
		$hc_hour = int $1;
		$hc_minute = int $2;

		# Now replace the HC hour and minute and zero seconds in the
		# current time, and see what the result is.
		$hc_t = mktime(0,$hc_minute,$hc_hour,$mday,$mon,$year);
		#print "hc_t=$hc_t\n";

		#print "Hour=$1\n";
		#print "Minute=$2\n";
		#print "hc_hour=$hc_hour\n";
		#print "hc_minute=$hc_minute\n";

		print sprintf("my time %ld %02d:%02d:%02d.%06d %04d/%02d/%02d\n",
			      $t, $hour, $min, $sec, $us,
			      $year+1900, $mon+1, $mday);
		print sprintf("HC time %ld %02d:%02d:00",
			      $hc_t, $hc_hour, $hc_minute);

		# Now find out the difference between the current time and what HC
		# thinks is the current time.
		$diff = $hc_t - ($t + ($us/1e6));
		print sprintf("                    %0.6f\n", $diff);

		if ($minute_counter == 1) {
			$cal_minute = "$1$2";
			$first_diff = $diff;
			$first_date = sprintf("%04d/%02d/%02d %02d:%02d",
					      $year+1900, $mon+1, $mday,
	 				      $hour, $min);
			print "\n\n----------------\n";
			print sprintf("Start: $first_date %10.6f\n", $first_diff);
			print "----------------\n\n";
		} else {
			if ("$1$2" eq $cal_minute) {
				$now_date = sprintf("%04d/%02d/%02d %02d:%02d",
						    $year+1900, $mon+1, $mday, $hour, $min);
				print "\n\n----------------\n";
				print sprintf("Start: $first_date %10.6f\n", $first_diff);
				print sprintf("Now  : $now_date %10.6f\n", $diff);
				print "----------------\n\n";
			}
		}

	}
}

# Local variables:
# perl-indent-level:8
# End:
