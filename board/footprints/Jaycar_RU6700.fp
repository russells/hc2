# Footprint for the Jaycar RU6700 0.047F (47000uF) 5.5V super capacitor.

#
# 5 mm lead spacing.
# 13 mm diameter.
# 1.3 mm lead width.
#
#
#   mm       thou
#   1.3      51.18
#   1.4      55.12
#   2,5      98.43
#   5.0     196.85
#   6.5     255.91
#  13.0     511.81
#


# Element [SFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TSFlags]

# Pin [rX rY Thickness Clearance Mask Drill "Name" "Number" SFlags]

# Pad [rX1 rY1 rX2 rY2 Thickness Clearance Mask "Name" "Number" SFlags]

# ElementLine[rX1 rY1 rX2 rY2 Thickness]

# ElementArc [rX rY Width Height StartAngle DeltaAngle Thickness]


#        S  D                         N    V  MX MY TX   TY    D S   F
Element ["" "Jaycar RU6700 super cap" "C?" "" 10 10 7000 7000  0 100 ""] (

Pin [     0     0 12000 1000 13000 5600 "1" "1" "square" ]
Pin [ 19685     0 12000 1000 13000 5600 "2" "2" "" ]

ElementArc [ 9843 0 25591 25591 0 360 1000 ]

)
