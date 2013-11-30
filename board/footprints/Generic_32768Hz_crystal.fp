# Footprint for a generic 32768Hz watch crysta..

#
# 10mil hole size.
# 0.7mm hole spacing.  But we can bend the pins out.
# 6mm length.
#
#  mm     mil
# 0.7   27.56
# 6.0  236.22


# Element [SFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TSFlags]

# Pin [rX rY Thickness Clearance Mask Drill "Name" "Number" SFlags]

# Pad [rX1 rY1 rX2 rY2 Thickness Clearance Mask "Name" "Number" SFlags]

# ElementLine[rX1 rY1 rX2 rY2 Thickness]

# ElementArc [rX rY Width Height StartAngle DeltaAngle Thickness]


#        S  D                         N    V  MX MY TX   TY    D S   F
Element ["" "Generic 32768Hz crystal" "X?" "" 10 10 1000 1500  0 100 ""] (

Pin [    0     0 2500 1000 3000 1000 "1" "1" "" ]
Pin [ 6000     0 2500 1000 3000 1000 "2" "2" "" ]

ElementLine [    0 10000    0 30000 1000 ]
ElementLine [ 6000 10000 6000 30000 1000 ]
ElementLine [    0 30000 6000 30000 1000 ]

Pin [ -3000 20000 2500 1000 3000 1000 "" "" "" ]
Pin [  9000 20000 2500 1000 3000 1000 "" "" "" ]

)
