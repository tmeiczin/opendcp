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
    t += "#define HEADROOM       %s\n" % (HEADROOM) 
    t += "#define DCI_LUT_SIZE   %s\n" % ("(COLOR_DEPTH + 1) * BIT_PRECISION")
    t += "#define DCI_GAMMA      %s\n" % (DCI_GAMMA)
    t += "#define DCI_DEGAMMA    %s\n" % ("1/DCI_GAMMA")
    t += "#define DCI_COEFFICENT %s"   % (DCI_COEFFICENT_S)

    return t

def enums():
    f = " = 0"
    t = "enum LUT_IN_ENUM {\n"
    for c in color_spaces_in:
        t += "    LI_%s%s,\n" % (c['id'].upper(), f)
        f = ''
    t += "    LI_MAX\n"
    t += "};\n\n"

    f = " = 0"
    t += "enum LUT_OUT_ENUM {\n"
    for c in color_spaces_out:
        t += "    LO_%s%s,\n" % (c['id'].upper(), f)
        f = ''
    t += "    LO_MAX\n"
    t += "};"

    return t

def gamma():
    l = len(color_spaces_in)

    t = "static float GAMMA[%d] = {\n" % (l)
    for i in range(0, l):
        c = color_spaces_in[i]
        t += "     %f,    /* %-8.8s */\n" % (c['gamma'], c['id'].upper())
    t += "};"

    return t
    
def color_matrix():
    l = len(color_spaces_in)

    t = "static float color_matrix[%d][3][3] = {\n" % (l)
    for i in range(0, l):
        c = color_spaces_in[i]
        t += "    /* %s */\n" % (c['id'].upper())
        t += "    {{%s},\n" % (", ".join(str(i) for i in c['matrix'][0]))
        t += "     {%s},\n" % (", ".join(str(i) for i in c['matrix'][1]))
        t += "     {%s}},\n" % (", ".join(str(i) for i in c['matrix'][2]))
        if i < l - 1:
            t += "\n"
    t += "};"

    return t
    
def footer():
    t = "#endif //_OPEN_DCP_XYZ_H_"

    return t

def test(source):
    for c in color_spaces_in:
       if c['id'] == source:
           print "found %s" % (c['id'])
           f = 'calculate_'+c['id']
           m = c['matrix']

    if not m: return

    for i in range(0, BIT_LENGTH):
        r = g = b = eval(f)(i)

        p = float(i) / (BIT_LENGTH - 1.0)
        if p > 0.08125:
           n = "*"
        else:
           n = ''

        xn = r*m[0][0] + g*m[0][1] + b*m[0][2]
        yn = r*m[1][0] + g*m[1][1] + b*m[1][2]
        zn = r*m[2][0] + g*m[2][1] + b*m[2][2]

        x = int(((xn*DCI_COEFFICENT) ** (1/DCI_GAMMA)) * (BIT_LENGTH - 1.0))
        y = int(((yn*DCI_COEFFICENT) ** (1/DCI_GAMMA)) * (BIT_LENGTH - 1.0))
        z = int(((zn*DCI_COEFFICENT) ** (1/DCI_GAMMA)) * (BIT_LENGTH - 1.0)) 

        print "%d, %d, %d | %f, %f, %f | %f, %f, %f | %f, %f, %f | %d, %d, %d | %s" % (i,i,i,p,p,p,r,g,b,xn,yn,zn,x,y,z,n)

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
    for x in range(0, BIT_LENGTH * BIT_PRECISION):
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

def calculate_srgb(i, complex=False):
    p = float(i) / (BIT_LENGTH - 1.0)
    
    if not complex:
        return p**SRGB_GAMMA

    if p > 0.04045:
        v = ((p+0.055)/1.055)**SRGB_GAMMA
    else:
        v = p/12.92

    return v

def calculate_srgb_complex(i):
    return calculate_srgb(i, complex=True)

def calculate_rec709(i, complex=False):
    p = float(i) / (BIT_LENGTH - 1.0) 

    if not complex:
        return p**REC709_GAMMA

    if p > 0.08125:
        v = ((p+0.099)/1.099)**REC709_GAMMA
    else: 
        v = p/4.50

    return v

def calculate_rec709_complex(i):
    return calculate_rec709(i, complex=True)

def calculate_p3(i, complex=False):
    p = float(i) / (BIT_LENGTH - 1.0)

    return p**DCI_GAMMA

def calculate_dci(i, complex=False):
    p = float(i) / (BIT_LENGTH - 1.0) / DCI_COEFFICENT
    v = p ** 1/DCI_GAMMA

    return v

def lut_in():
    t = "static float lut_in[LI_MAX][COLOR_DEPTH+1] = {\n"
    for i in range(0, len(color_spaces_in)):
        c = color_spaces_in[i]
        t += "    // Bit Depth:       %s\n" % (BIT_DEPTH)
        t += "    // Reference White: %s\n" % (c['id'].upper())
        t += "    // Gamma:           %s\n" % (c['gamma'])
        t += "    {\n";
        t += generate_lut_in(c['id'])
        t += "    },\n"
        if i < len(color_spaces_in) - 1:
            t += "\n"
    t += "};"

    return t

def lut_out():
    t  = "static int lut_out[1][DCI_LUT_SIZE] = {\n"
    t += "    // Bit Depth:       %s\n" % (BIT_DEPTH)
    t += "    // Reference White: %s\n" % ("DCI")
    t += "    // Gamma:           %s\n" % (DCI_GAMMA)
    t += "    {\n"
    t += generate_lut_out("dci")
    t += "    }\n"
    t += "};"

    return t

color_spaces_in = [
                { 
                 'id':     'srgb',
                 'gamma':  2.4,
                 'matrix': [[0.4124564, 0.3575761, 0.1804375],
                            [0.2126729, 0.7151522, 0.0721750],
                            [0.0193339, 0.1191920, 0.9503041]]},
                { 
                 'id':     'rec709',
                 'gamma':  1.0/0.45,
                 'matrix': [[0.4123907993, 0.3575843394, 0.1804807884],
                            [0.2126390059, 0.7151686778, 0.0721923154],
                            [0.0193308187, 0.1191947798, 0.9505321522]]},
                {
                 'id':     'p3',
                 'gamma':  2.6,
                 'matrix': [[0.4451698156, 0.2771344092, 0.1722826698],
                            [0.2094916779, 0.7215952542, 0.0689130679],
                            [0.0000000000, 0.0470605601, 0.9073553944]]},
                {
                 'id':     'srgb_complex',
                 'gamma':  2.4,
                 'matrix': [[0.4124564, 0.3575761, 0.1804375],
                            [0.2126729, 0.7151522, 0.0721750],
                            [0.0193339, 0.1191920, 0.9503041]]},
                {
                 'id':     'rec709_complex',
                 'gamma':  1.0/0.45,
                 'matrix': [[0.4123907993, 0.3575843394, 0.1804807884],
                            [0.2126390059, 0.7151686778, 0.0721923154],
                            [0.0193308187, 0.1191947798, 0.9505321522]]},
               ]

color_spaces_out = [
                    {
                    'id':    'dci',
                    'gamma': 1/2.6,
                    'matrix': [[0.4123907993, 0.3575843394, 0.1804807884],
                               [0.2126390059, 0.7151686778, 0.0721923154],
                               [0.0193308187, 0.1191947798, 0.9505321522]]},
                    {
                    'id':    'srgb',
                    'gamma': 1/2.4,
                    'matrix': [[ 3.2404542, -1.5371385, -0.4985314],
                               [-0.9692660,  1.8760108,  0.0415560],
                               [ 0.0556434, -0.2040259,  1.0572252]]},
                   ]

BIT_DEPTH       = 12 
HEADROOM        = int(16 * 2**(BIT_DEPTH-8))
COLOR_DEPTH     = 2**BIT_DEPTH - 1
BIT_PRECISION   = 16
BIT_LENGTH      = 2**BIT_DEPTH
DCI_GAMMA       = 2.6
SRGB_GAMMA      = 2.4
REC709_GAMMA    = 1/.45 
DCI_COEFFICENT  = 48.0/52.37
DCI_COEFFICENT_S = "48.0/52.37"

if len(sys.argv) > 1: 
    if sys.argv[1] == "test":
        test("rec709")
        #test("srgb")
    sys.exit(1)

print "%s\n" % (header)
print "%s\n" % (defines())
print "%s\n" % (enums())
print "%s\n" % (gamma())
print "%s\n" % (color_matrix())
print "%s\n" % (lut_in())
print "%s\n" % (lut_out())
print "%s"   % (footer())
