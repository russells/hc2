# Footprint for the Jaycar SP0611 tactile SPST switch.

#
#
#
#                                     ---
#                      UU              ^
#                    ------     ---    |
#                   |      |     ^     |
#                   |      |     |     |
#                   |      |     |     |
#                   |      |
#                   |      |     6    7.9
#                   |      |
#                   |      |     |     |
#                   |      |     |     |
#                   |      |     v     |
#                    ------     ---    |
#                      UU              v
#                                     ---
#
#
#                  -->|  |<-- 0.7
#
#                   |<---->| 3.5
#
#
#
#
#
#
#   mm         mil
#  ----      --------
#   0.7         27.56
#   1.0         39.37
#   3.0        118.11
#   4.5        177.17
#   5.5        216.54
#   6.0        236.22
#   6.5        255.91
#   7.5        295.28
#   7.9        311.02
#  22.0        866.14
#


# Element [SFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TSFlags]

# Pin [rX rY Thickness Clearance Mask Drill "Name" "Number" SFlags]


# ElementLine[rX1 rY1 rX2 rY2 Thickness]


#        S  D                        N    V  MX MY TX   TY  D S   F
Element ["" "Jaycar SP0611 switch" "S?" "" 10 10 6000 0   0 100 ""] (

Pin [     0     0 6000 1000 8000 2900 "1" "1" "square" ]
Pin [     0 25591 6000 1000 8000 2900 "2" "2" "" ]
#Pin [ 17716     0 6000 1000 8000 2900 "2" "2" "" ]
#Pin [ 17716 25591 6000 1000 8000 2900 "" "" "" ]

ElementLine [ -3937 -3937  3937 -3937 1000 ]
ElementLine [  3937 -3937  3937 29528 1000 ]
ElementLine [  3937 29528 -3937 29528 1000 ]
ElementLine [ -3937 29528 -3937 -3937 1000 ]

)
