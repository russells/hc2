# Footprint for the 6x1 FTDI header.

#
# 01. inch lead spacing.
#


# Element [SFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TSFlags]

# Pin [rX rY Thickness Clearance Mask Drill "Name" "Number" SFlags]

# Pad [rX1 rY1 rX2 rY2 Thickness Clearance Mask "Name" "Number" SFlags]

# ElementLine[rX1 rY1 rX2 rY2 Thickness]

# ElementArc [rX rY Width Height StartAngle DeltaAngle Thickness]


#        S  D             N       V  MX MY TX    TY     D S   F
Element ["" "FTDI header" "CONN?" "" 10 10 10000 10000  0 100 ""] (

Pin [ 0      0 6000 1000 6400 3800 "1" "1" "square" ]
Pin [ 0  10000 6000 1000 6400 3800 "2" "2" "" ]
Pin [ 0  20000 6000 1000 6400 3800 "3" "3" "" ]
Pin [ 0  30000 6000 1000 6400 3800 "4" "4" "" ]
Pin [ 0  40000 6000 1000 6400 3800 "5" "5" "" ]
Pin [ 0  50000 6000 1000 6400 3800 "6" "6" "" ]

ElementLine [ -5000 -5000 40000 -5000 1000 ]
ElementLine [ 40000 -5000 40000 55000 1000 ]
ElementLine [ 40000 55000 -5000 55000 1000 ]
ElementLine [ -5000 55000 -5000 -5000 1000 ]
ElementLine [  5000 -5000  5000 55000 1000 ]

)
