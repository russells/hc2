#!/usr/bin/env python

# Print a table that converts from an ADC reading for the MSP430F449
# integrated temperature sensor with the 1.5V reference, to a scaled
# temperature value.

def t_to_v(t):
    return (t * 0.00335) + 0.986

def v_to_adc(v):
    return int((v / 1.5) * 4096)

def t_to_ti(t):
    return int(t * 2.0)

mintemp = -10.0
maxtemp =  60.0

print "static const struct TempConversion tempconversions[] = {"
t = mintemp

nconversions = 0

while t <= maxtemp:
    vmin = t_to_v(t)
    vmax = t_to_v(t+0.5)
    adcmin = v_to_adc(vmin)
    adcmax = v_to_adc(vmax) - 1
    ti = int(t * 2.0);
    print "\t{ %d, %d, %d, }, /* %.1fC, %.5fV, %.5fV */" % \
        (adcmin, adcmax, ti, t, vmin, vmax)
    t += 0.5
    nconversions += 1

print "};"

print "#define MINTI  %d" % t_to_ti(mintemp)
print "#define MINADC %d" % v_to_adc(t_to_v(mintemp))
print "#define MAXTI  %d" % t_to_ti(maxtemp)
print "#define MAXADC %d" % v_to_adc(t_to_v(maxtemp))
print "#define NCONVERSIONS %d" % nconversions
