#!/usr/bin/python

import sys

header = '''\
/* 
    OpenDCP: Builds Digital Cinema Packages
    Copyright (c) 2010-2014 Terrence Meiczinger, All Rights Reserved

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/\
'''

def defines():
    t  = "#ifndef _OPEN_DCP_XYZ_H_\n"
    t += "#define _OPEN_DCP_XYZ_H_\n\n"
    t += "#define BIT_DEPTH      %s\n" % (BIT_DEPTH)
    t += "#define BIT_PRECISION  %s\n" % (BIT_PRECISION)
    t += "#define COLOR_DEPTH    %s\n" % (COLOR_DEPTH) 
    t += "#define DCI_LUT_SIZE   %s\n" % ("(COLOR_DEPTH + 1) * BIT_PRECISION")
    t += "#define DCI_GAMMA      %s\n" % (DCI_GAMMA)
    t += "#define DCI_DEGAMMA    %s\n" % ("1/DCI_GAMMA")
    t += "#define DCI_COEFFICENT %s"   % (DCI_COEFFICENT)

    return t

enums = '''\
enum COLOR_PROFILE_ENUM {
    CP_SRGB = 0,
    CP_REC709,
    CP_DC28,
    CP_MAX
};

enum LUT_IN_ENUM {
    LI_SRGB    = 0,
    LI_REC709,
    LI_MAX
};

enum LUT_OUT_ENUM {
    LO_DCI    = 0,
    LO_MAX
};\
'''

color_matrix = '''\
static float GAMMA[3] = {
    2.4,    /* SRGB    */
    2.2,    /* REC709  */
    2.6     /* DCI     */
};

/* Color Matrices */
static float color_matrix[3][3][3] = {
    /* SRGB */
    {{0.4124564, 0.3575761, 0.1804375},
     {0.2126729, 0.7151522, 0.0721750},
     {0.0193339, 0.1191920, 0.9503041}},

    /* REC.709 */
    {{0.4124564, 0.3575761, 0.1804375},
     {0.2126729, 0.7151522, 0.0721750},
     {0.0193339, 0.1191920, 0.9503041}},

    /* DC28.30 (2006-02-24) */
    {{0.4451698156, 0.2771344092, 0.1722826698},
     {0.2094916779, 0.7215952542, 0.0689130679},
     {0.0000000000, 0.0470605601, 0.9073553944}}
};\
'''

footer = "#endif //_OPEN_DCP_XYZ_H_"

def calculate_srgb(i):
    p = float(i) / (BIT_LENGTH - 1.0)

    if p > 0.04045:
        v = ((p+0.055)/1.055)**SRGB_GAMMA
    else:
        v = p/12.92

    return v

def calculate_rec709(i):
    p = float(i) / (BIT_LENGTH - 1.0)

    if p > 0.08125:
        v = ((p+0.099)/1.099)**REC709_GAMMA
    else: 
        v = p/4.50

    return v

def test():
    rec709 = color_spaces['rec709']['matrix']
    print "bit length: %s" % (BIT_LENGTH)
    for i in range(160, 165):
        r = g = b = calculate_rec709(i)

        x = r*rec709[0][0] + g*rec709[0][1] + b*rec709[0][2]
        y = r*rec709[1][0] + g*rec709[1][1] + b*rec709[1][2]
        z = r*rec709[2][0] + g*rec709[2][1] + b*rec709[2][2]

        x = int(((x*DCI_COEFFICENT) ** (1/DCI_GAMMA)) * (BIT_LENGTH - 1.0) - 256)
        y = int(((y*DCI_COEFFICENT) ** (1/DCI_GAMMA)) * (BIT_LENGTH - 1.0) - 256)
        z = int(((z*DCI_COEFFICENT) ** (1/DCI_GAMMA)) * (BIT_LENGTH - 1.0) - 256) 

        print "(%d, %d, %d) -> (%d, %d, %d)" % (i,i,i,x,y,x)

def generate_lut_in(lut):
    t = ''
    f = 'calculate_'+lut

    c = 0;
    for x in range(0, BIT_LENGTH):
        v = eval(f)(x)

        if c == 0:
            t += "    "

        if x < BIT_LENGTH - 1:
            t += "%06f," % (v)
            if c == 16:
                c = 0
                t += "\n"
            else:
                c = c + 1
        else:
            t += "%06f" %(v) 

    return t 

def generate_lut_out(lut):
    c = 0
    t = ''
    for x in range(0, BIT_LENGTH*BIT_PRECISION):
        v = int(((x/(BIT_LENGTH * BIT_PRECISION - 1.0)) ** (1/DCI_GAMMA)) * (BIT_LENGTH - 1.0))

        if c == 0:
            t += "    "

        if x < BIT_LENGTH * BIT_PRECISION - 1:
            t += "%4d," % (v)
        else:
            t += "%4d" % (v)

        if c == 25:
             c = 0
             t += "\n"
        else:
            c = c + 1 

    return t

def calculate_srgb(i):
    p = float(i) / (BIT_LENGTH - 1.0)

    if p > 0.04045:
        v = ((p+0.055)/1.055)**SRGB_GAMMA
    else:
        v = p/12.92

    return v

def calculate_rec709(i):
    p = float(i) / (BIT_LENGTH - 1.0)

    if p > 0.08125:
        v = ((p+0.099)/1.099)**REC709_GAMMA
    else: 
        v = p/4.50

    return v

color_spaces = {'rec709': {'name': "REC. 709", 'gamma': 1.0/0.45, 'matrix': [[0.4124564, 0.3575761, 0.1804375],[0.4124564, 0.3575761, 0.1804375], [0.0193339, 0.1191920, 0.9503041]]},
                'srgb':   {'name': "sRGB", 'gamma': 2.4, 'matrix': [[]]}
               }

def print_lut_in():
    t = "static float lut_in[LI_MAX][COLOR_DEPTH+1] = {\n"
    for key in color_spaces:
        t += "    // Bit Depth:       %s\n" % (BIT_DEPTH)
        t += "    // Reference White: %s\n" % (color_spaces[key]['name'])
        t += "    // Gamma:           %s\n" % (color_spaces[key]['gamma'])
        t += "    {\n";
        t += generate_lut_in(key)
        t += "    },\n\n"
    t += "};\n"

    return t

def print_lut_out():
    t  = "static int lut_out[1][DCI_LUT_SIZE] = {\n"
    t += "    // Bit Depth:       %s\n" % (BIT_DEPTH)
    t += "    // Reference White: %s\n" % ("DCI")
    t += "    // Gamma:           %s\n" % (DCI_GAMMA)
    t += "    {\n"
    t += generate_lut_out("dci")
    t += "    }\n"
    t += "};\n"

    return t

BIT_DEPTH      = 12
COLOR_DEPTH    = 2**BIT_DEPTH - 1
BIT_PRECISION  = 16
BIT_LENGTH     = 2**BIT_DEPTH
DCI_GAMMA      = 2.6
SRGB_GAMMA     = 2.4
REC709_GAMMA   = 1/0.45
DCI_COEFFICENT = 48.0/52.37

test()
sys.exit(1)

print "%s\n" % (header)
print "%s\n" % (defines())
print "%s\n" % (enums)
print "%s\n" % (color_matrix)
print "%s\n" % (print_lut_in())
print "%s\n" % (print_lut_out())
print "%s" % (footer)
