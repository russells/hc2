# Named VIM828FP.fp instead of VIM828.fp to avoid any possible naming
# conflict with VIM828.sym.

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
#                              25.00 mm = 984.25 mil
#                    |<-------------------------------------->|
#
#            Pin 1   O                                 -----  O  Pin 36
#                                                        ^
#                    O                                   |    O
#                                                        |
#                    O  -----                            |    O
#                         |  2.54 mm = 100 mil           |
#                    O  -----                                 O
#                                                   20.32 mm
#                    O                            = 800 mil   O
#
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        |
#                    O                                   |    O
#                                                        v
#            Pin 9   O  -----                          -----  O  Pin 28
#                         ^
#                         |
#                         |
#                         |
#                         |
#                         |
#                         |
#
#                  22.86mm = 900 mil
#
#                         |
#                         |
#                         |
#                         |
#                         |
#                         |
#                         V
#            Pin 10  O  -----                                 O  Pin 27
#                         |  2.54 mm = 100 mil
#                    O  -----                                 O
#
#                    O                                        O
#
#                    O                                        O
#
#                    O                                        O
#
#                    O                                        O
#
#                    O                                        O
#
#                    O                                        O
#
#            Pin 18  O                                        O  Pin 19
#
#
#

# NOTES:

# 1. Make sure to do all measurements in 1/100 mil units.  (ie 1e-5 inch,
# or 0.000254mm).

# 2. To match (1), use square brackets everywhere.



# Element [SFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TSFlags]

Element ["" "Varitronix VIM-828 LCD" "" "" 5000 5000 -1000 1000 0 100 ""] (

#       Pin [rX  rY   Thickness Clearance Mask Drill "Name" "Number" SFlags]

	Pin [ 0   0      5000      1000   3000  2500  "1"     "1"      "square"]
	Pin [ 0   10000  5000      1000   3000  2500  "2"     "2"      ""]
	Pin [ 0   20000  5000      1000   3000  2500  "3"     "3"      ""]
	Pin [ 0   30000  5000      1000   3000  2500  "4"     "4"      ""]
	Pin [ 0   40000  5000      1000   3000  2500  "5"     "5"      ""]
	Pin [ 0   50000  5000      1000   3000  2500  "6"     "6"      ""]
	Pin [ 0   60000  5000      1000   3000  2500  "7"     "7"      ""]
	Pin [ 0   70000  5000      1000   3000  2500  "8"     "8"      ""]
	Pin [ 0   80000  5000      1000   3000  2500  "9"     "9"      ""]

	Pin [ 0  170000  5000      1000   3000  2500  "10"    "10"     ""]
	Pin [ 0  180000  5000      1000   3000  2500  "11"    "11"     ""]
	Pin [ 0  190000  5000      1000   3000  2500  "12"    "12"     ""]
	Pin [ 0  200000  5000      1000   3000  2500  "13"    "13"     ""]
	Pin [ 0  210000  5000      1000   3000  2500  "14"    "14"     ""]
	Pin [ 0  220000  5000      1000   3000  2500  "15"    "15"     ""]
	Pin [ 0  230000  5000      1000   3000  2500  "16"    "16"     ""]
	Pin [ 0  240000  5000      1000   3000  2500  "17"    "17"     ""]
	Pin [ 0  250000  5000      1000   3000  2500  "18"    "18"     ""]

	Pin [ 98425  250000  5000      1000   3000  2500  "19"    "19"     ""]
	Pin [ 98425  240000  5000      1000   3000  2500  "20"    "20"     ""]
	Pin [ 98425  230000  5000      1000   3000  2500  "21"    "21"     ""]
	Pin [ 98425  220000  5000      1000   3000  2500  "22"    "22"     ""]
	Pin [ 98425  210000  5000      1000   3000  2500  "23"    "23"     ""]
	Pin [ 98425  200000  5000      1000   3000  2500  "24"    "24"     ""]
	Pin [ 98425  190000  5000      1000   3000  2500  "25"    "25"     ""]
	Pin [ 98425  180000  5000      1000   3000  2500  "26"    "26"     ""]
	Pin [ 98425  170000  5000      1000   3000  2500  "27"    "27"     ""]

	Pin [ 98425   80000  5000      1000   3000  2500  "28"     "28"      ""]
	Pin [ 98425   70000  5000      1000   3000  2500  "29"     "29"      ""]
	Pin [ 98425   60000  5000      1000   3000  2500  "30"     "30"      ""]
	Pin [ 98425   50000  5000      1000   3000  2500  "31"     "31"      ""]
	Pin [ 98425   40000  5000      1000   3000  2500  "32"     "32"      ""]
	Pin [ 98425   30000  5000      1000   3000  2500  "33"     "33"      ""]
	Pin [ 98425   20000  5000      1000   3000  2500  "34"     "34"      ""]
	Pin [ 98425   10000  5000      1000   3000  2500  "35"     "35"      ""]
	Pin [ 98425   0      5000      1000   3000  2500  "36"     "36"      ""]

)
