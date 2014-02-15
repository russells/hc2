#!/usr/bin/env python

# Create rtc-add-sub.inc, containing RTC calibration information.

import sys

max_adj = 80

inc_file_name = "rtc-add-sub.inc"
ui_adj_file_name = "ui-rtc-adj.inc"

inc_f = open(inc_file_name, "w")

# We need an array of ADJ_MAX arrays of struct TimeOnly.  At each time, 1/8
# second is adjusted in the RTC.  We adjust in steps of 1/4 second per day,
# so we need twice as many adjustments per day as there are 1/4 seconds to
# do.

print >>inc_f, "Q_ASSERT_COMPILE( MAX_ADJ == %d );" % max_adj

print >>inc_f, "static const struct TimeOnly adjustments_0[] = { };"

for n in range(1,max_adj+1):
    print >>inc_f, "static const struct TimeOnly adjustments_%d[] = {" % n
    div = 1440 / (n*2)
    print >>inc_f, "// div=%d" % div
    for minutecounter in range(n*2):
        minutes = div * minutecounter
        hour = minutes / 60
        minute = minutes - (hour*60)
        s = "%02d%02d" % (hour, minute)
        print >>inc_f, "\t{ '%s', '%s', '%s', '%s' }, // m=%d minutes=%d" % \
              (s[0], s[1], s[2], s[3], minutecounter, minutes)
    print >>inc_f, "};"

print >>inc_f, "static const struct TimeOnly *adjustments[] = {"
for n in range(0,max_adj+1):
    print >>inc_f, "\tadjustments_%d," % n
print >>inc_f, "};"
inc_f.close()


ui_f = open(ui_adj_file_name, "w")

print >>ui_f, "Q_ASSERT_COMPILE( MAX_ADJ == %d );" % max_adj
print >>ui_f, '''
/**
 * Strings to display to indicate the time adjustment.
 *
 * We can adjust the time (clock speed) in 0.25 second increments, from -20 to
 * +20 seconds per day.  These strings indicate each step, and they must match
 * the definitions of MIN_ADJ and MAX_ADJ.  The use of those sizes in the array
 * size helps here.
 *
 * Each string must be seven characters long, to match the LCD size.
 */
'''
print >>ui_f, "static const char *adjuststrings[1 + MAX_ADJ - MIN_ADJ] = {"
for n in range(-max_adj, max_adj+1):
    s = "%+05.2f" % (n * 0.25)
    if len(s) == 5:
        # eg -20.00
        print >>ui_f, '"%c0\\xb%c" "%c%c", /* "%s" */' % (s[0], s[1], s[3], s[4], s)
    else:
        # eg -9.75
        print >>ui_f, '"%c%c\\xb%c" "%c%c", /* "%s" */' % (s[0], s[1], s[2], s[4], s[5], s)
print >>ui_f, "};"
ui_f.close()
