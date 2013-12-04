# Footprint for the Keystone 1020 SMT AAA battery holder.


#
# GENERAL OUTLINE
#
#               +----------------------------------------+
#               |         ^                              |             |
#               |         |                              |             v
#     +---------+         |                              +---------+  ---
#     |         |        500                             |         |
#     |         |         |                              |         |
#     |         |         |                              |         |
#     +---------+         |                              +---------+  ---
#               |         |                              |             ^
#               |         V                              |             |
#               +----------------------------------------+            200
#     |                                                            |
#     |         |                                        |         |
#     |         |<-------------------- 2140 ------------>|         |
#     |         |                                        |         |
#     |                                                            |
#     |<-------------------------- 2421 -------------------------->|
#
#
# RECOMMENDED PADS
#
#             78 dia                                     |         |
#                O  ---                      250 (min) ->|         |<-
#                    ^                                   |         |   |
#                    |                                                 v
#     +----------+   | 209                               +---------+  ---
#     |          |   v                                   |         |
#     |          |  ---                                  |         |
#     |          |                                       |         |
#     +----------+                                       +---------+  ---
#                                                                      ^
#     |                                                            |   |
#     |<------------------- 2421 (min) --------------------------->|   |
#     |                                                            |   |
#                                                                      |
#                                                                   200 (min)
#

# Make the pads larger than the recommended minimum by increasing the
# "Thickness" attribute of the pad.  The pads are specified as 50 thou
# long, which is the difference between the recommended minimum width and
# length.  If the Thickness is the minimum width, then the length works out
# also as its minimum.

# The alignment hole is 80 thou, instead of the min 78 thou.



# Element [SFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TSFlags]

# Pin [rX rY Thickness Clearance Mask Drill "Name" "Number" SFlags]

# Pad [rX1 rY1 rX2 rY2 Thickness Clearance Mask "Name" "Number" SFlags]

# ElementLine[rX1 rY1 rX2 rY2 Thickness]

#        S  D                              N    V  MX MY  TX      TY  D S   F
Element ["" "Keystone 1020 SMT AAA Holder" "B?" "" 10 10 -5000 -20000 0 100 ""] (

        Pad [  -2500 0   2500 0 22000 4000 23000 "1" "1" "square" ]
        Pad [ 214600 0 219600 0 22000 4000 23000 "2" "2" "square" ]

        # Alignment hole.
        Pin [ 12500 -20900 12000 1000 13000 8000 "" "" "" ]

        # "+" marker
        ElementLine [     0 12000     0 22000 1000 ]
        ElementLine [ -5000 17000  5000 17000 1000 ]

        # Outline, avoiding the alignment hole.
        ElementLine [  12500  15000  12500  25000 1000 ]
        ElementLine [  12500  25000 204500  25000 1000 ]
        ElementLine [ 204500  25000 204500  15000 1000 ]
        ElementLine [ 204500 -15000 204500 -25000 1000 ]
        ElementLine [ 204500 -25000  25000 -25000 1000 ]
)
