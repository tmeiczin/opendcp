#!/usr/bin/perl

sub print_header() {
print
"/*
    OpenDCP: Builds Digital Cinema Packages
    Copyright (c) 2010-2011 Terrence Meiczinger, All Rights Reserved

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
*/\n";
}

sub print_defines() {
print
"
#ifndef _OPEN_DCP_XYZ_H_
#define _OPEN_DCP_XYZ_H_

#define BIT_DEPTH      $BIT_DEPTH 
#define BIT_PRECISION  $BIT_PRECISION 
#define COLOR_DEPTH    ($COLOR_DEPTH)
#define DCI_LUT_SIZE   ((COLOR_DEPTH + 1) * BIT_PRECISION)
#define DCI_GAMMA      ($DCI_GAMMA)
#define DCI_DEGAMMA    (1/DCI_GAMMA)
#define DCI_COEFFICENT ($DCI_COEFFICENT)
\n";
}

sub print_enums() {
print 
"enum COLOR_PROFILE_ENUM {
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
};
\n";
}

sub print_color_matrix {
print
"static float GAMMA[3] = {
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
};
\n";
}

sub print_footer {
    print "\n#endif //_OPEN_DCP_XYZ_H_\n";
}

sub calculate_srgb {
    my $lx =    shift;

    if ( $lx > 0.04045) {
        $v = (($lx+0.055)/1.055)**$SRGB_GAMMA;
    } else {
        $v = $lx/12.92;
    }

    return $v;
}

sub calculate_rec709 {
    my $lx    = shift;

    if ( $lx > 0.08125) {
        $v = (($lx+0.099)/1.099)**$REC709_GAMMA;
    } else {
        $v = $lx/4.50;
    }

    return $v;
}

sub generate_lut_in {
    my $lut = shift;
    my $gamma = shift;
    my $x, $c, $v;

    my $func = "calculate_$lut";

    $c = 0;
    for ($x = 0; $x < $BIT_LENGTH; $x++) {
        $v = $x/($BIT_LENGTH - 1.0);
        $v = &$func($v, $gamma);

        if ($c == 0) {
            print "    ";
        }

        if ($x < $BIT_LENGTH - 1) {
            printf "%06f,",$v; 
            if ($c == 15) {
                $c = 0;
                print "\n";
            } else {
                $c++;
            }
        } else {
            printf "%06f",$v; 
        }
    } 
}

sub generate_lut_out {
    my $lut = shift;
    my $x;

    my $c = 0;

    for ($x = 0; $x < ($BIT_LENGTH*$BIT_PRECISION); $x++) {
        $v = int((($x/($BIT_LENGTH * $BIT_PRECISION - 1.0)) ** (1/$DCI_GAMMA)) * ($BIT_LENGTH - 1.0));

        if ($c == 0) {
            print "    ";
        }

        if ($x < ($BIT_LENGTH*$BIT_PRECISION) - 1) {
            printf "%4d,",$v;
        } else {
            printf "%4d",$v;
        }

        if ($c == 25) {
             $c = 0;
             print "\n";
        } else {
            $c++;
        }
    }
}

sub print_lut_in {
    my $x, $lx, $v;

    print "static float lut_in[LI_MAX][$COLOR_DEPTH+1] = {\n";

    print "    // Bit Depth:       $BIT_DEPTH\n";
    print "    // Reference White: sRGB\n";
    print "    // Gamma:           $SRGB_GAMMA\n";
    print "    {\n";
    generate_lut_in("srgb");
    print "\n    },\n";

    print "\n";
    print "    // Bit Depth:       $BIT_DEPTH\n";
    print "    // Reference White: REC.709\n";
    print "    // Gamma:           $REC709_GAMMA\n";
    print "    {\n";
    generate_lut_in("rec709");
    print "\n    },\n";

    print "};\n\n";
}

sub print_lut_out {
    my $x, $lx, $v;

    print "static int lut_out[1][DCI_LUT_SIZE] = {\n";
    print "    // Bit Depth:       $BIT_DEPTH\n";
    print "    // Reference White: DCI\n";
    print "    // Gamma:           $DCI_GAMMA\n";
    print "    {\n";
    generate_lut_out("dci");
    print "\n    }\n";

    print "};\n";
}

#### MAIN ####

$BIT_DEPTH      = 12;
$COLOR_DEPTH    = 2**$BIT_DEPTH - 1;
$BIT_PRECISION  = 16;
$BIT_LENGTH     = 2**$BIT_DEPTH;
$DCI_GAMMA      = 2.6;
$SRGB_GAMMA     = 2.4;
$REC709_GAMMA   = 1/0.45;
$DCI_COEFFICENT = "48.0/52.37";

print_header();
print_defines();
print_enums();
print_color_matrix();
print_lut_in();
print_lut_out();
print_footer();
