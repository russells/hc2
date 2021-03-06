# Named SBLCDA2FP.fp instead of SBLCDA2.fp to avoid any possible naming
# conflict with SBLCDA2.sym.

# A document describing how to make footprints:
# http://www.brorson.com/gEDA/land_patterns_20070818.pdf

# 1. The mark will be the centre of pin 1.

# 2. Text rotation will be 0.

# 3. Grid placement courtyard does not matter (hand assembly.)

# 4. Hand soldering with through holes.

# 5. Pad locations and sizes from the document.

# 6. Solder mask ?

# 7. Solder mask relief size ?

#
# Layout in ASCII art:
#
#
#
#
#
#                              27.94 mm = 1100 mil
#                    |<-------------------------------------->|
#
#            Pin 1   O                                 -----  O  Pin 48
#                                                        ^
#                    O                                   |    O
#                                                        |
#                    O  -----                            |    O
#                         |   2.0 mm = 78.74 mil         |
#                    O  -----                                 O
#                                                    46 mm
#                    O                        = 1811.02 mil   O
#
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        v
#            Pin 24  O                                 -----  O  Pin 25
#
#

# NOTES:

# 1. Make sure to do all measurements in 1/100 mil units.  (ie 1e-5 inch,
# or 0.000254mm).

# 2. To match (1), use square brackets everywhere.



# Element [SFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TSFlags]


# Pin [rX rY Thickness Clearance Mask Drill "Name" "Number" SFlags]



# +++++ Generated by ./SBLCDA2FP-generator.sh +++++

Element ["" "SBLCDA2 LCD" "U?" "" 10 10 10000 0 0 100 ""] (
	Pin [      0      0 5000 1000 5500 2900 "1" "1" "square" ]
	Pin [      0   7874 5000 1000 5500 2900 "2" "2" "" ]
	Pin [      0  15748 5000 1000 5500 2900 "3" "3" "" ]
	Pin [      0  23622 5000 1000 5500 2900 "4" "4" "" ]
	Pin [      0  31496 5000 1000 5500 2900 "5" "5" "" ]
	Pin [      0  39370 5000 1000 5500 2900 "6" "6" "" ]
	Pin [      0  47244 5000 1000 5500 2900 "7" "7" "" ]
	Pin [      0  55118 5000 1000 5500 2900 "8" "8" "" ]
	Pin [      0  62992 5000 1000 5500 2900 "9" "9" "" ]
	Pin [      0  70866 5000 1000 5500 2900 "10" "10" "" ]
	Pin [      0  78740 5000 1000 5500 2900 "11" "11" "" ]
	Pin [      0  86614 5000 1000 5500 2900 "12" "12" "" ]
	Pin [      0  94488 5000 1000 5500 2900 "13" "13" "" ]
	Pin [      0 102362 5000 1000 5500 2900 "14" "14" "" ]
	Pin [      0 110236 5000 1000 5500 2900 "15" "15" "" ]
	Pin [      0 118110 5000 1000 5500 2900 "16" "16" "" ]
	Pin [      0 125984 5000 1000 5500 2900 "17" "17" "" ]
	Pin [      0 133858 5000 1000 5500 2900 "18" "18" "" ]
	Pin [      0 141732 5000 1000 5500 2900 "19" "19" "" ]
	Pin [      0 149606 5000 1000 5500 2900 "20" "20" "" ]
	Pin [      0 157480 5000 1000 5500 2900 "21" "21" "" ]
	Pin [      0 165354 5000 1000 5500 2900 "22" "22" "" ]
	Pin [      0 173228 5000 1000 5500 2900 "23" "23" "" ]
	Pin [      0 181102 5000 1000 5500 2900 "24" "24" "" ]
	Pin [ 110000 181102 5000 1000 5500 2900 "25" "25" "" ]
	Pin [ 110000 173228 5000 1000 5500 2900 "26" "26" "" ]
	Pin [ 110000 165354 5000 1000 5500 2900 "27" "27" "" ]
	Pin [ 110000 157480 5000 1000 5500 2900 "28" "28" "" ]
	Pin [ 110000 149606 5000 1000 5500 2900 "29" "29" "" ]
	Pin [ 110000 141732 5000 1000 5500 2900 "30" "30" "" ]
	Pin [ 110000 133858 5000 1000 5500 2900 "31" "31" "" ]
	Pin [ 110000 125984 5000 1000 5500 2900 "32" "32" "" ]
	Pin [ 110000 118110 5000 1000 5500 2900 "33" "33" "" ]
	Pin [ 110000 110236 5000 1000 5500 2900 "34" "34" "" ]
	Pin [ 110000 102362 5000 1000 5500 2900 "35" "35" "" ]
	Pin [ 110000  94488 5000 1000 5500 2900 "36" "36" "" ]
	Pin [ 110000  86614 5000 1000 5500 2900 "37" "37" "" ]
	Pin [ 110000  78740 5000 1000 5500 2900 "38" "38" "" ]
	Pin [ 110000  70866 5000 1000 5500 2900 "39" "39" "" ]
	Pin [ 110000  62992 5000 1000 5500 2900 "40" "40" "" ]
	Pin [ 110000  55118 5000 1000 5500 2900 "41" "41" "" ]
	Pin [ 110000  47244 5000 1000 5500 2900 "42" "42" "" ]
	Pin [ 110000  39370 5000 1000 5500 2900 "43" "43" "" ]
	Pin [ 110000  31496 5000 1000 5500 2900 "44" "44" "" ]
	Pin [ 110000  23622 5000 1000 5500 2900 "45" "45" "" ]
	Pin [ 110000  15748 5000 1000 5500 2900 "46" "46" "" ]
	Pin [ 110000   7874 5000 1000 5500 2900 "47" "47" "" ]
	Pin [ 110000      0 5000 1000 5500 2900 "48" "48" "" ]

# ElementLine[rX1 rY1 rX2 rY2 Thickness]

	ElementLine [  -5000  -5000  -5000 186102 1000 ]
	ElementLine [  -5000 186102 115000 186102 1000 ]
	ElementLine [ 115000 186102 115000  -5000 1000 ]
	ElementLine [ 115000  -5000  -5000  -5000 1000 ]

	ElementLine [  -5000   4000   5000   4000 1000 ]
	ElementLine [   5000   4000   5000  -5000 1000 ]

)
# ---------------------------
